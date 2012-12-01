#import "DLDevice.h"
#import "micronucleus_lib.h"

static void printProgress(float progress) {
    
}

@implementation DLDevice

@synthesize delegate;

- (id)uploadMembers {
    int length;
    int start = 0;
    
	NSLog(@"updating firmware");

    [delegate progress: 1.0 label: @"Please Plug Device In"];
    device = NULL;
    while (device == NULL) {
        device = micronucleus_connect();
    }
    [delegate progress: 50.0 label: @"Reading"];
    
    if (device->page_size == 64) {
        NSLog(@"found device");
    } else {
        return 0;
    }
    
    do {
        length = micronucleus_readEeprom(device, start, printProgress);
        start = start + length;
    } while (length > 0);
    NSLog(@"> Done reading eeprom\n");
    
    [delegate progress: 100.0 label: @"Done"];

    return 0;
}

- (id)updateFirmware {
    
    return 0;
}
@end
