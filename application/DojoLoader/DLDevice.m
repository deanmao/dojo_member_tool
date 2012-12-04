#import "DLDevice.h"
#import "micronucleus_lib.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fmemopen.h"

static void printProgress(float progress) {
    
}

static int parseUntilColon(FILE *file_pointer) {
    int character;
    
    do {
        character = getc(file_pointer);
    } while(character != ':' && character != EOF);
    
    return character;
}

static long parseHex(FILE *file_pointer, int num_digits) {
    int iter;
    char temp[9];
    
    for(iter = 0; iter < num_digits; iter++) {
        temp[iter] = getc(file_pointer);
    }
    temp[iter] = 0;
    
    return strtol(temp, NULL, 16);
}

@implementation DLDevice

@synthesize delegate;

- (id)waitForDevice {
    [delegate progress: 1.0 label: @"Please Plug Device In"];
    device = NULL;
    while (device == NULL) {
        device = micronucleus_connect();
        if ([delegate cancelled]) {
            return 0;
        }
    }
    NSLog(@"end address = %d", device->data_length);
    return 0;
}

- (id)uploadMembers:(NSArray*)members {
    NSLog(@"updating firmware");

    [self waitForDevice];

    if ([delegate cancelled]) {
        return 0;
    }
    [delegate progress: 10.0 label: @"Writing"];

    unsigned char data[10000];
    int dataIndex = 0;
    for(NSString *key in members) {
        long long k = [key longLongValue];
        NSLog(@"k = %.10lld", k);

        for(int i=0; i<5; i++) {
            unsigned char b = k & 0xFF;
            data[dataIndex++] = b;
            NSLog(@"writing: %d", b);
            k = k >> 8;
        }
        data[dataIndex++] = 0;
        NSLog(@"writing: %d", 0);
        if ([delegate cancelled]) {
            return 0;
        }
    }
    
    micronucleus_writeEeprom(device, data, dataIndex, printProgress);
    
    [delegate progress: 20.0 label: @"Verifying data"];
    
    int end = (int) [members count] * 6;
    int length;
    int start = 0;
    do {
        unsigned char buffer[128];
        if ([delegate cancelled]) {
            return 0;
        }
        length = micronucleus_readEeprom(device, start, end, buffer, printProgress);
        for (int i=0; i<length; i++) {
            NSLog(@"reading: %d == %d", buffer[i], data[start + i]);
            if (buffer[i] != data[start + i]) {
                [delegate failure: @"Verification failed!"];
                return 0;
            }
        }
        start = start + length;
    } while (length > 0);
    
    [delegate success: @"Done"];

    return 0;
}

- (id)updateFirmware:(void*)str {
    unsigned char buffer[65536 + 256];
    int res;
    memset(buffer, 0xFF, sizeof(buffer));

    [delegate progress: 10.0 label: @"Reading hex file"];

    long startAddress = 1, endAddress = 0;
    long address, base, d, segment, i, lineLen, sum;
    
    FILE *input;
    
    input = fmemopen (str, strlen(str), "r");
    
    while (parseUntilColon(input) == ':') {
        if ([delegate cancelled]) {
            return 0;
        }
        sum = 0;
        sum += lineLen = parseHex(input, 2);
        base = address = parseHex(input, 4);
        sum += address >> 8;
        sum += address;
        sum += segment = parseHex(input, 2);  /* segment value? */
        if (segment != 0) {   /* ignore lines where this byte is not 0 */
            continue;
        }
        
        for (i = 0; i < lineLen; i++) {
            d = parseHex(input, 2);
            buffer[address++] = d;
            sum += d;
        }
        
        sum += parseHex(input, 2);
        if ((sum & 0xff) != 0) {
            printf("> Warning: Checksum error between address 0x%lx and 0x%lx, blah: %lx\n", base, address, (sum & 0xff));
        }
        
        if(startAddress > base) {
            startAddress = base;
        }
        if(endAddress < address) {
            endAddress = address;
        }
    }
    
    fclose(input);

    if (startAddress >= endAddress) {
        [delegate failure: @"No data in firmware file"];
        return 0;
    }
    
    [self waitForDevice];
    if ([delegate cancelled]) {
        return 0;
    }
    if (endAddress > device->flash_size) {
        [delegate failure: @"Firmware is too large to fit on chip"];
        return 0;
    }
    
    if ([delegate cancelled]) {
        return 0;
    }
    [delegate progress: 20.0 label: @"Erasing memory"];
    res = micronucleus_eraseFlash(device, printProgress);
    if (res != 0) {
        [delegate failure: @"An error occured when erasing chip"];
        return 0;
    }
    
    if ([delegate cancelled]) {
        return 0;
    }
    [delegate progress: 20.0 label: @"Uploading firmware"];
    res = micronucleus_writeFlash(device, (int) endAddress, buffer, printProgress);
    if (res != 0) {
        [delegate failure: @"An error occured when uploading the firmware"];
        return 0;
    }

    [delegate success: @"Done"];
    return 0;
}
@end
