#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h> 
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <Arduino.h>

// pins 1 & 2 are used for I2C
#define TWI_SDA_PIN PB1
#define TWI_SCL_PIN PB2

#define EEPROM_ADDR 0x50

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#define I2C_DELAY_USEC 4
#define I2C_READ 1
#define I2C_WRITE 0

// pin 0 is the serial rx for our rfid reader
SoftwareSerial rfid = SoftwareSerial(0, 5);
char data[16];
bool readingRfid = false;
byte dataIndex = 0;


// Initialize SCL/SDA pins and set the bus high
void SoftI2cMasterInit(void) {
  DDRB |= (1<<TWI_SDA_PIN);
  PORTB |= (1<<TWI_SDA_PIN);
  DDRB |= (1<<TWI_SCL_PIN);
  PORTB |= (1<<TWI_SCL_PIN);
}

// De-initialize SCL/SDA pins and set the bus low
void SoftI2cMasterDeInit(void) {
  PORTB &= ~(1<<TWI_SDA_PIN);
  DDRB &= ~(1<<TWI_SDA_PIN);
  PORTB &= ~(1<<TWI_SCL_PIN);
  DDRB &= ~(1<<TWI_SCL_PIN);
}

// Read a byte from I2C and send Ack if more reads follow else Nak to terminate read
uint8_t SoftI2cMasterRead(uint8_t last) {
  uint8_t b = 0;
  // Make sure pull-up enabled
  PORTB |= (1<<TWI_SDA_PIN);
  DDRB &= ~(1<<TWI_SDA_PIN);
  // Read byte
  for (uint8_t i = 0; i < 8; i++) {
    // Don't change this loop unless you verify the change with a scope
    b <<= 1;
    _delay_us(I2C_DELAY_USEC);
    PORTB |= (1<<TWI_SCL_PIN);
    if (bit_is_set(PINB, TWI_SDA_PIN)) b |= 1;
    PORTB &= ~(1<<TWI_SCL_PIN);
  }
  // Send Ack or Nak
  DDRB |= (1<<TWI_SDA_PIN);
  if (last) { 
    PORTB |= (1<<TWI_SDA_PIN); 
  }
  else { 
    PORTB &= ~(1<<TWI_SDA_PIN);
  }  
  PORTB |= (1<<TWI_SCL_PIN);
  _delay_us(I2C_DELAY_USEC);
  PORTB &= ~(1<<TWI_SCL_PIN);
  PORTB &= ~(1<<TWI_SDA_PIN);
  return b;
}

// Write a byte to I2C
bool SoftI2cMasterWrite(uint8_t data) {
  // Write byte
  for (uint8_t m = 0x80; m != 0; m >>= 1) {
    // Don't change this loop unless you verify the change with a scope
    if (m & data) { 
      PORTB |= (1<<TWI_SDA_PIN); 
    }
    else { 
      PORTB &= ~(1<<TWI_SDA_PIN); 
    }
    PORTB |= (1<<TWI_SCL_PIN);
    _delay_us(I2C_DELAY_USEC);
    PORTB &= ~(1<<TWI_SCL_PIN);
  }
  // get Ack or Nak
  DDRB &= ~(1<<TWI_SDA_PIN);
  // Enable pullup
  PORTB |= (1<<TWI_SDA_PIN);
  PORTB |= (1<<TWI_SCL_PIN);
  uint8_t rtn = bit_is_set(PINB, TWI_SDA_PIN);
  PORTB &= ~(1<<TWI_SCL_PIN);
  DDRB |= (1<<TWI_SDA_PIN);
  PORTB &= ~(1<<TWI_SDA_PIN);
  return rtn == 0;
}

// Issue a start condition
bool SoftI2cMasterStart(uint8_t addressRW) {
  PORTB &= ~(1<<TWI_SDA_PIN);
  _delay_us(I2C_DELAY_USEC);
  PORTB &= ~(1<<TWI_SCL_PIN);
  return SoftI2cMasterWrite(addressRW);
}

// Issue a restart condition
bool SoftI2cMasterRestart(uint8_t addressRW) {
  PORTB |= (1<<TWI_SDA_PIN);
  PORTB |= (1<<TWI_SCL_PIN);
  _delay_us(I2C_DELAY_USEC);
  return SoftI2cMasterStart(addressRW);
}

// Issue a stop condition
void SoftI2cMasterStop(void) {
  PORTB &= ~(1<<TWI_SDA_PIN);
  _delay_us(I2C_DELAY_USEC);
  PORTB |= (1<<TWI_SCL_PIN);
  _delay_us(I2C_DELAY_USEC);
  PORTB |= (1<<TWI_SDA_PIN);
  _delay_us(I2C_DELAY_USEC);
}

bool find(byte *tag, byte length) {
  int i;
  byte x = 0;
  int matchedBytes = 0;
  boolean found = false;
  int bytestoRead = EEPROM.read(10) + (EEPROM.read(11) << 8);

  if (!SoftI2cMasterStart((EEPROM_ADDR<<1) | I2C_WRITE)) return false;
  if (!SoftI2cMasterWrite(0)) return false; // MSB
  if (!SoftI2cMasterWrite(0)) return false; // LSB
  if (!SoftI2cMasterRestart((EEPROM_ADDR<<1) | I2C_READ)) return false;

  for (i = 0; i < bytestoRead; i++) {
    // we compare byte by byte and ensure the length is the right
    // amount
    byte c = SoftI2cMasterRead(i == (bytestoRead-1));
    if (((i + 1)%6) == 0) {
      // we are at a boundary, if found is still true, we matched everything
      if (matchedBytes == 5) {
        SoftI2cMasterStop();
        return true;
      } else {
        matchedBytes = 0;
      }
    } else {
      if (c == tag[i%6]) {
        matchedBytes++;
      } else {
        matchedBytes = 0;
      }
    }
  }
  SoftI2cMasterStop();

  return false;
}

// pins 3 & 4 are green & red leds
byte green = 4;
byte red = 3;

void clearLed() {
  digitalWrite(green, LOW);
  digitalWrite(red, LOW);
}

// This block is commented out because we don't have a serial
// port on the attiny85, but when we test it we can use the atmega328p
// or alternative processors.
//
//void print(byte tag[], byte len) {
//  byte i;
//  for(i=0; i<len; i++) {
//    char buf[12];
//    rfid.write(itoa(tag[i], buf, 10));
//    rfid.write(' ');
//  }
//}


//void loop() {
//  while(rfid.available()) {
//    byte readByte = rfid.read() - 64;
//    data[dataIndex] = readByte;
//    dataIndex++;
//    if (dataIndex == 14) {
//      dataIndex = 0;
//      byte i;
//      for(i=0; i<14; i++) {
//        rfid.write(data[i]);
//      }
//    }
//  }
//}

void loop() {

}

int __attribute__((noreturn)) main(void) {
  sei();
  SoftI2cMasterInit();
  rfid.begin(9600);
  pinMode(green, OUTPUT); 
  pinMode(red, OUTPUT); 
  for(byte i=0; i<5; i++) {
    digitalWrite(green, HIGH);
    digitalWrite(red, LOW);
    delay(80);
    digitalWrite(green, LOW);
    digitalWrite(red, HIGH);
    delay(80);
  }
  clearLed();

  while(1) {
    while(rfid.available()) {
      byte readByte = rfid.read();

      if (readByte == 2) {
        readingRfid = true;
      }

      if( readingRfid ){
        if (readByte != 2 && readByte != 3 && dataIndex < 16) {
          if (dataIndex > 1) {
            // the data that comes from the rfid reader is actually ascii
            // text, so we have to convert the ascii into normal hex
            byte b = readByte - 112;
            if (b > 9) {
              b = b - 7;
            }
            // it is dataIndex-2 because the second byte is where
            // the actual number starts
            data[dataIndex-2] = b;
          }
          dataIndex++;
        } 
        else if (readByte == 3) {
          byte byteTag[6];
          byte i, t;
          byte last = dataIndex-4;
          readingRfid = false;
          dataIndex = 0;
          for(i=0; i<6; i++) {
            byteTag[i] = 0; 
          }
          for(i=0, t=3; i<last; i+=2, t--) {
            if (t >= 0) byteTag[t] = (data[i]<<4) + data[i+1];
          }
          for(i=0; i<16; i++) {
            data[i] = 0;
          }
          if (find(byteTag, t)) {
            clearLed();
            digitalWrite(green, HIGH);
            delay(1000);
            digitalWrite(green, LOW);
          } 
          else {
            clearLed();
            digitalWrite(red, HIGH);
            delay(1000);
            digitalWrite(red, LOW);
          }
        }
      }
    }
  }
}