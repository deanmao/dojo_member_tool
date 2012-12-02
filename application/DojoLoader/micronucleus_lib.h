#ifndef MICRONUCLEUS_LIB_H
#define MICRONUCLEUS_LIB_H


/*
  Created: September 2012
  by ihsan Kehribar <ihsan@kehribar.me>
  modifications from Dean Mao <deanmao@gmail.com>
  
  Permission is hereby granted, free of charge, to any person obtaining a copy of
  this software and associated documentation files (the "Software"), to deal in
  the Software without restriction, including without limitation the rights to
  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
  of the Software, and to permit persons to whom the Software is furnished to do
  so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.  
*/


#include "usb.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MICRONUCLEUS_VENDOR_ID   0x16D0
#define MICRONUCLEUS_PRODUCT_ID  0x0753
#define MICRONUCLEUS_USB_TIMEOUT 0xFFFF
#define MICRONUCLEUS_MAX_MAJOR_VERSION 1

typedef struct _micronucleus_version {
  unsigned char major;
  unsigned char minor;
} micronucleus_version;

typedef struct _micronucleus {
  usb_dev_handle *device;
  micronucleus_version version;

  unsigned int flash_size;  // programmable size (in bytes) of progmem
  unsigned int data_length;
  unsigned int page_size;   // size (in bytes) of page
  unsigned int pages;       // total number of pages to program
  unsigned int write_sleep; // milliseconds
  unsigned int erase_sleep; // milliseconds
} micronucleus;

typedef void (*micronucleus_callback)(float progress);
micronucleus* micronucleus_connect();
void delay(unsigned int duration);
int micronucleus_eraseFlash(micronucleus* deviceHandle, micronucleus_callback progress);
int micronucleus_writeEeprom(micronucleus* deviceHandle, unsigned char* data, int length, micronucleus_callback prog);
int micronucleus_readEeprom(micronucleus* deviceHandle, int address, int length, unsigned char *buffer, micronucleus_callback progress);
int micronucleus_writeFlash(micronucleus* deviceHandle, unsigned int program_length,
                            unsigned char* program, micronucleus_callback progress);
int micronucleus_startApp(micronucleus* deviceHandle);

#endif
