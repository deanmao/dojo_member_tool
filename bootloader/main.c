/* Name: main.c
 * Project: Micronucleus
 * Author: Jenna Fox
 * Creation Date: 2007-12-08
 * Tabsize: 4
 * Copyright: (c) 2012 Jenna Fox
 * Portions Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH (USBaspLoader)
 * Portions Copyright: (c) 2012 Louis Beaudoin (USBaspLoader-tiny85)
 * License: GNU GPL v2 (see License.txt)
 */
 
#define MICRONUCLEUS_VERSION_MAJOR 1
#define MICRONUCLEUS_VERSION_MINOR 4
// how many milliseconds should host wait till it sends another erase or write?
// needs to be above 4.5 (and a whole integer) as avr freezes for 4.5ms
#define MICRONUCLEUS_WRITE_SLEEP 6


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <avr/sleep.h>
//#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/eeprom.h>
//#include <string.h>

static void leaveBootloader() __attribute__((__noreturn__));

#include "bootloaderconfig.h"
#include "usbdrv/usbdrv.c"

#define TWI_SDA_PIN PB1
#define TWI_SCL_PIN PB2

#define ledPin (1<<PB0)
#define EEPROM_ADDR 0x50

#include "myeeprom.c"

/* ------------------------------------------------------------------------ */

#ifndef ulong
#   define ulong    unsigned long
#endif
#ifndef uint
#   define uint     unsigned int
#endif

#ifndef BOOTLOADER_CAN_EXIT
#   define  BOOTLOADER_CAN_EXIT     0
#endif

/* allow compatibility with avrusbboot's bootloaderconfig.h: */
#ifdef BOOTLOADER_INIT
#   define bootLoaderInit()         BOOTLOADER_INIT
#   define bootLoaderExit()
#endif
#ifdef BOOTLOADER_CONDITION
#   define bootLoaderCondition()    BOOTLOADER_CONDITION
#endif

/* device compatibility: */
#ifndef GICR    /* ATMega*8 don't have GICR, use MCUCR instead */
#   define GICR     MCUCR
#endif

/* ------------------------------------------------------------------------ */

#define addr_t uint

// typedef union longConverter{
//     addr_t  l;
//     uint    w[sizeof(addr_t)/2];
//     uchar   b[sizeof(addr_t)];
// } longConverter_t;

//////// Stuff Bluebie Added
// postscript are the few bytes at the end of programmable memory which store tinyVectors
// and used to in USBaspLoader-tiny85 store the checksum iirc
#define POSTSCRIPT_SIZE 4
#define PROGMEM_SIZE (BOOTLOADER_ADDRESS - POSTSCRIPT_SIZE) /* max size of user program */

// verify the bootloader address aligns with page size
#if BOOTLOADER_ADDRESS % SPM_PAGESIZE != 0
#  error "BOOTLOADER_ADDRESS in makefile must be a multiple of chip's pagesize"
#endif

#ifdef AUTO_EXIT_MS
#  if AUTO_EXIT_MS < (MICRONUCLEUS_WRITE_SLEEP * (BOOTLOADER_ADDRESS / SPM_PAGESIZE))
#    warning "AUTO_EXIT_MS is shorter than the time it takes to perform erase function - might affect reliability?"
#    warning "Try increasing AUTO_EXIT_MS if you have stability problems"
#  endif
#endif

// events system schedules functions to run in the main loop
static uchar events = 0; // bitmap of events to run
#define EVENT_ERASE_APPLICATION 1
#define EVENT_WRITE_PAGE 2
#define EVENT_EXECUTE 4
#define EVENT_CONTINUE_POLLING 8
#define EVENT_START_WRITING_EEPROM 16
#define EVENT_WRITE_EEPROM 32

#define EEPROM_PAGE_SIZE 64
#define SEND_BUFFER_SIZE 128

// controls state of events
#define fireEvent(event) events |= (event)
#define isEvent(event)   (events & (event))
#define clearEvents()    events = 0

// length of bytes to write in to flash memory in upcomming usbFunctionWrite calls
//static unsigned char writeLength;

// becomes 1 when some programming happened
// lets leaveBootloader know if needs to finish up the programming
static uchar didWriteSomething = 0;

uint16_t idlePolls = 0; // how long have we been idle?
enum { FALSE = 0, TRUE };
uchar continuePolling = FALSE;
uchar writeBuffer[64];
uint16_t writeBufferIndex = 0;
uint16_t eepromAddress = 0;
uint16_t totalWriteCount, currentWriteCount;
static uchar writingToEeprom = FALSE;

static uint16_t vectorTemp[2]; // remember data to create tinyVector table before BOOTLOADER_ADDRESS
static addr_t currentAddress; // current progmem address, used for erasing and writing


/* ------------------------------------------------------------------------ */
static inline void eraseApplication(void);
static inline void startWritingEeprom(void);
static void writeFlashPage(void);
static void writeWordToPageBuffer(uint16_t data);
static void fillFlashWithVectors(void);
static uchar usbFunctionSetup(uchar data[8]);
static uchar usbFunctionWrite(uchar *data, uchar length);
static inline void initForUsbConnectivity(void);
static inline void tiny85FlashInit(void);
static inline void tiny85FlashWrites(void);
//static inline void tiny85FinishWriting(void);
static inline void leaveBootloader(void);

// erase any existing application and write in jumps for usb interrupt and reset to bootloader
//  - Because flash can be erased once and programmed several times, we can write the bootloader
//  - vectors in now, and write in the application stuff around them later.
//  - if vectors weren't written back in immidately, usb would fail.
static inline void eraseApplication(void) {
    // erase all pages until bootloader, in reverse order (so our vectors stay in place for as long as possible)
    // while the vectors don't matter for usb comms as interrupts are disabled during erase, it's important
    // to minimise the chance of leaving the device in a state where the bootloader wont run, if there's power failure
    // during upload
    currentAddress = BOOTLOADER_ADDRESS;
    cli();
    while (currentAddress) {
        currentAddress -= SPM_PAGESIZE;
        
        boot_page_erase(currentAddress);
        boot_spm_busy_wait();
    }
    
    fillFlashWithVectors();
    sei();
}

// simply write currently stored page in to already erased flash memory
static void writeFlashPage(void) {
    didWriteSomething = 1;
    cli();
    boot_page_write(currentAddress - 2);
    boot_spm_busy_wait(); // Wait until the memory is written.
    sei();
}

// clear memory which stores data to be written by next writeFlashPage call
#define __boot_page_fill_clear()   \
(__extension__({                                 \
    __asm__ __volatile__                         \
    (                                            \
        "sts %0, %1\n\t"                         \
        "spm\n\t"                                \
        :                                        \
        : "i" (_SFR_MEM_ADDR(__SPM_REG)),        \
          "r" ((uint8_t)(__BOOT_PAGE_FILL | (1 << CTPB)))     \
    );                                           \
}))

// write a word in to the page buffer, doing interrupt table modifications where they're required
static void writeWordToPageBuffer(uint16_t data) {
    // first two interrupt vectors get replaced with a jump to the bootloader's vector table
    if (currentAddress == (RESET_VECTOR_OFFSET * 2) || currentAddress == (USBPLUS_VECTOR_OFFSET * 2)) {
        data = 0xC000 + (BOOTLOADER_ADDRESS/2) - 1;
    }
    
    // at end of page just before bootloader, write in tinyVector table
    // see http://embedded-creations.com/projects/attiny85-usb-bootloader-overview/avr-jtag-programmer/
    // for info on how the tiny vector table works
    if (currentAddress == BOOTLOADER_ADDRESS - TINYVECTOR_RESET_OFFSET) {
        data = vectorTemp[0] + ((FLASHEND + 1) - BOOTLOADER_ADDRESS)/2 + 2 + RESET_VECTOR_OFFSET;
    }
    
    if (currentAddress == BOOTLOADER_ADDRESS - TINYVECTOR_USBPLUS_OFFSET) {
        data = vectorTemp[1] + ((FLASHEND + 1) - BOOTLOADER_ADDRESS)/2 + 1 + USBPLUS_VECTOR_OFFSET;
    }
    
    
    // clear page buffer as a precaution before filling the buffer on the first page
    // in case the bootloader somehow ran after user program and there was something
    // in the page buffer already
    if (currentAddress == 0x0000) __boot_page_fill_clear();
    
    cli();
    boot_page_fill(currentAddress, data);
    sei();
    
	// only need to erase if there is data already in the page that doesn't match what we're programming
	// TODO: what about this: if (pgm_read_word(currentAddress) & data != data) { ??? should work right?
	//if (pgm_read_word(currentAddress) != data && pgm_read_word(currentAddress) != 0xFFFF) {
    //if ((pgm_read_word(currentAddress) & data) != data) {
    //    fireEvent(EVENT_PAGE_NEEDS_ERASE);
    //}
    
    // increment progmem address by one word
    currentAddress += 2;
}

// fills the rest of this page with vectors - interrupt vector or tinyvector tables where needed
static void fillFlashWithVectors(void) {
    //int16_t i;
    //
    // fill all or remainder of page with 0xFFFF (as if unprogrammed)
    //for (i = currentAddress % SPM_PAGESIZE; i < SPM_PAGESIZE; i += 2) {
    //    writeWordToPageBuffer(0xFFFF); // is where vector tables are sorted out
    //}
    
    // TODO: Or more simply: 
    do {
        writeWordToPageBuffer(0xFFFF);
    } while (currentAddress % SPM_PAGESIZE);

    writeFlashPage();
}

/* ------------------------------------------------------------------------ */


static uchar usbFunctionSetup(uchar data[8]) {
    usbRequest_t *rq = (void *)data;
    idlePolls = 0; // reset idle polls when we get usb traffic
    
    static uchar replyBuffer[6] = {
        (((uint)PROGMEM_SIZE) >> 8) & 0xff,
        ((uint)PROGMEM_SIZE) & 0xff,
        SPM_PAGESIZE,
        MICRONUCLEUS_WRITE_SLEEP,0,2
    };
    
    if (rq->bRequest == 0) { // get device info
        replyBuffer[4] = eeprom_read_byte(10);
        replyBuffer[5] = eeprom_read_byte(11); 
        usbMsgPtr = replyBuffer;
        return 6;
        
    } else if (rq->bRequest == 1) { // write page
        //writeLength = rq->wValue.word;
        currentAddress = rq->wIndex.word;
        
        return USB_NO_MSG; // hands off work to usbFunctionWrite
        
    } else if (rq->bRequest == 2) { // erase application
        fireEvent(EVENT_ERASE_APPLICATION);
        
    } else if (rq->bRequest == 3) { // Dojo: Read eeprom
      uint16_t startAddress = rq->wValue.word;
      uint16_t endAddress = rq->wIndex.word;
      uint16_t length = SEND_BUFFER_SIZE;
      byte dataBuffer[SEND_BUFFER_SIZE];

      writeBufferIndex = 0;
      currentWriteCount = 0;
      writingToEeprom = FALSE;
      eepromAddress = 0;

      if (endAddress == 0) {
        length = 0;
      } else if ((endAddress - startAddress) > SEND_BUFFER_SIZE) {
        length = SEND_BUFFER_SIZE;
      } else {
        length = endAddress - startAddress;
      }
      if (length > 0) {
        soft_i2c_eeprom_read_bytes(EEPROM_ADDR, startAddress, dataBuffer, length);
        usbMsgPtr = dataBuffer;
      }
      return length;
    } else if (rq->bRequest == 4) { // Dojo: Write eeprom
      totalWriteCount = rq->wValue.word;
      fireEvent(EVENT_WRITE_EEPROM);
      return USB_NO_MSG;
    } else if (rq->bRequest == 5) { // Dojo: Write eeprom
      fireEvent(EVENT_START_WRITING_EEPROM);
      return 0;
    } else { // exit bootloader
#       if BOOTLOADER_CAN_EXIT
            fireEvent(EVENT_EXECUTE);
#       endif
    }
    
    return 0;
}

static inline void startWritingEeprom(void) {
  writeBufferIndex = 0;
  currentWriteCount = 0;
  writingToEeprom = TRUE;
  eepromAddress = 0;
  eeprom_write_word(10, 0);
}

static void writeEeprom(uchar len) {
  soft_i2c_eeprom_write_bytes(EEPROM_ADDR, eepromAddress, writeBuffer, writeBufferIndex, len);
  eepromAddress = eepromAddress + len;
  eeprom_write_word(10, eepromAddress);
}

// read in a page over usb, and write it in to the flash write buffer
static uchar usbFunctionWrite(uchar *data, uchar length) {
  uchar i;

  if (writingToEeprom == TRUE) {
    for(i = 0; i<length; i++) {
      writeBuffer[writeBufferIndex] = data[i];
      writeBufferIndex++;
      currentWriteCount++;
      if (writeBufferIndex == EEPROM_PAGE_SIZE || currentWriteCount == totalWriteCount) {
        writeEeprom(writeBufferIndex);
        writeBufferIndex = 0;
      }
    }
    if (currentWriteCount == totalWriteCount) {
      writingToEeprom = FALSE;
      return 1;
    } else {
      return 0;
    }
  } else {
      //if (length > writeLength) length = writeLength; // test for missing final page bug
      //writeLength -= length;
    
      do {
          // remember vectors or the tinyvector table 
          if (currentAddress == RESET_VECTOR_OFFSET * 2) {
              vectorTemp[0] = *(short *)data;
          }
    
          if (currentAddress == USBPLUS_VECTOR_OFFSET * 2) {
              vectorTemp[1] = *(short *)data;
          }
    
          // make sure we don't write over the bootloader!
          if (currentAddress >= BOOTLOADER_ADDRESS) {
              //__boot_page_fill_clear();
              break;
          }
    
          writeWordToPageBuffer(*(uint16_t *) data);
          data += 2; // advance data pointer
          length -= 2;
      } while(length);
    
      // if we have now reached another page boundary, we're done
      //uchar isLast = (writeLength == 0);
      uchar isLast = ((currentAddress % SPM_PAGESIZE) == 0);
      // definitely need this if! seems usbFunctionWrite gets called again in future usbPoll's in the runloop!
      if (isLast) fireEvent(EVENT_WRITE_PAGE); // ask runloop to write our page
    
      return isLast; // let vusb know we're done with this request
    }
}

/* ------------------------------------------------------------------------ */

void PushMagicWord (void) __attribute__ ((naked)) __attribute__ ((section (".init3")));

// put the word "B007" at the bottom of the stack (RAMEND - RAMEND-1)
void PushMagicWord (void) {
    asm volatile("ldi r16, 0xB0"::);
    asm volatile("push r16"::);
    asm volatile("ldi r16, 0x07"::);
    asm volatile("push r16"::);
}

/* ------------------------------------------------------------------------ */

static inline void initForUsbConnectivity(void) {
    usbInit();
    /* enforce USB re-enumerate: */
    usbDeviceDisconnect();  /* do this while interrupts are disabled */
    _delay_ms(500);
    usbDeviceConnect();
    sei();
}

static inline void tiny85FlashInit(void) {
    // check for erased first page (no bootloader interrupt vectors), add vectors if missing
    // this needs to happen for usb communication to work later - essential to first run after bootloader
    // being installed
    if(pgm_read_word(RESET_VECTOR_OFFSET * 2) != 0xC000 + (BOOTLOADER_ADDRESS/2) - 1 ||
            pgm_read_word(USBPLUS_VECTOR_OFFSET * 2) != 0xC000 + (BOOTLOADER_ADDRESS/2) - 1) {

        fillFlashWithVectors();
    }

    // TODO: necessary to reset currentAddress?
    currentAddress = 0;
}

static inline void tiny85FlashWrites(void) {
    _delay_us(2000); // TODO: why is this here? - it just adds pointless two level deep loops seems like?
    // write page to flash, interrupts will be disabled for > 4.5ms including erase
    
    // TODO: Do we need this? Wouldn't the programmer always send full sized pages?
    if (currentAddress % SPM_PAGESIZE) { // when we aren't perfectly aligned to a flash page boundary
        fillFlashWithVectors(); // fill up the rest of the page with 0xFFFF (unprogrammed) bits
    } else {
        writeFlashPage(); // otherwise just write it
    }
}

// finishes up writing to the flash, including adding the tinyVector tables at the end of memory
// TODO: can this be simplified? EG: currentAddress = PROGMEM_SIZE; fillFlashWithVectors();
// static inline void tiny85FinishWriting(void) {
//     // make sure remainder of flash is erased and write checksum and application reset vectors
//     if (didWriteSomething) {
//         while (currentAddress < BOOTLOADER_ADDRESS) {
//             fillFlashWithVectors();
//         }
//     }
// }

// reset system to a normal state and launch user program
static inline void leaveBootloader(void) {
    _delay_ms(10); // removing delay causes USB errors
    
    //DBG1(0x01, 0, 0);
    bootLoaderExit();
    cli();
    USB_INTR_ENABLE = 0;
    USB_INTR_CFG = 0;       /* also reset config bits */

    // clear magic word from bottom of stack before jumping to the app
    *(uint8_t*)(RAMEND) = 0x00;
    *(uint8_t*)(RAMEND-1) = 0x00;

    // jump to application reset vector at end of flash
    asm volatile ("rjmp __vectors - 4");
}

int __attribute__((noreturn)) main(void) {
    /* initialize  */
    #ifdef RESTORE_OSCCAL
        uint8_t osccal_default = OSCCAL;
    #endif
    #if (!SET_CLOCK_PRESCALER) && LOW_POWER_MODE
        uint8_t prescaler_default = CLKPR;
    #endif
    
    wdt_disable();      /* main app may have enabled watchdog */
    tiny85FlashInit();
    bootLoaderInit();
    SoftI2cMasterInit();
    DDRB |= ledPin;
    
    if (bootLoaderStartCondition()) {
        #if LOW_POWER_MODE
            // turn off clock prescalling - chip must run at full speed for usb
            // if you might run chip at lower voltages, detect that in bootLoaderStartCondition
            CLKPR = 1 << CLKPCE;
            CLKPR = 0;
        #endif
        
        initForUsbConnectivity();
        do {
            usbPoll();
            _delay_us(100);
            idlePolls++;
            
            // these next two freeze the chip for ~ 4.5ms, breaking usb protocol
            // and usually both of these will activate in the same loop, so host
            // needs to wait > 9ms before next usb request
            if (isEvent(EVENT_ERASE_APPLICATION)) eraseApplication();
            if (isEvent(EVENT_WRITE_PAGE)) tiny85FlashWrites();
            if (isEvent(EVENT_START_WRITING_EEPROM)) startWritingEeprom();

            if (isEvent(EVENT_CONTINUE_POLLING)) continuePolling = TRUE;
#           if BOOTLOADER_CAN_EXIT            
                if (isEvent(EVENT_EXECUTE)) { // when host requests device run uploaded program
                    break;
                }
#           endif
            
            clearEvents();
            
        } while(bootLoaderCondition());  /* main event loop runs so long as bootLoaderCondition remains truthy */
    }
    
    // set clock prescaler to desired clock speed (changing from clkdiv8, or no division, depending on fuses)
    #if LOW_POWER_MODE
        #ifdef SET_CLOCK_PRESCALER
            CLKPR = 1 << CLKPCE;
            CLKPR = SET_CLOCK_PRESCALER;
        #else
            CLKPR = 1 << CLKPCE;
            CLKPR = prescaler_default;
        #endif
    #endif
    
    SoftI2cMasterDeInit();
    
    // Go back to 8mhz because we need to use SoftwareSerial which doesn't work great for any other speeds
    CLKPR = 1 << CLKPCE;
    CLKPR = _BV(CLKPS0);
            
    // slowly bring down OSCCAL to it's original value before launching in to user program
    #ifdef RESTORE_OSCCAL
        while (OSCCAL > osccal_default) { OSCCAL -= 1; }
    #endif
    leaveBootloader();
}

/* ------------------------------------------------------------------------ */
