/*
 AVR Soft I2C Master - Example for I2C EEPROMs
 Version: 1.0
 Author: Alex from insideGadgets (www.insidegadgets.com)
 Created: 12/02/2012
 Last Modified: 12/02/2012
 
 Using code from http://forums.adafruit.com/viewtopic.php?f=25&t=13722

 */

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#ifndef boolean
	typedef uint8_t boolean;
#endif 
#ifndef bool
	typedef uint8_t bool;
#endif 
#ifndef byte
	typedef uint8_t byte;
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#define LOW 0
#define HIGH 1
#define false 0
#define true 1

#define I2C_DELAY_USEC 4
#define I2C_READ 1
#define I2C_WRITE 0

uint16_t something = 0;

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
  uint8_t i;
  
	// Make sure pull-up enabled
	PORTB |= (1<<TWI_SDA_PIN);
	DDRB &= ~(1<<TWI_SDA_PIN);
  
	// Read byte
	for (i = 0; i < 8; i++) {
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
  uint8_t m;
  
	// Write byte
	for (m = 0x80; m != 0; m >>= 1) {
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

// // Read 1 byte from the EEPROM device and return it
// uint8_t soft_i2c_eeprom_read_byte(uint8_t deviceAddr, uint16_t readAddress) {
//  uint8_t byteRead = 0;
//  
//  // Issue a start condition, send device address and write direction bit
//  if (!SoftI2cMasterStart((deviceAddr<<1) | I2C_WRITE)) return false;
// 
//  if (!SoftI2cMasterWrite((readAddress >> 8))) return false; // MSB
//  if (!SoftI2cMasterWrite((readAddress & 0xFF))) return false; // LSB
// 
//  // Issue a repeated start condition, send device address and read direction bit
//  if (!SoftI2cMasterRestart((deviceAddr<<1) | I2C_READ)) return false;
//  
//  // Read the byte
//  byteRead = SoftI2cMasterRead(1);
// 
//  // Issue a stop condition
//  SoftI2cMasterStop();
//  
//  return byteRead;
// }

// // Write 1 byte to the EEPROM
// bool soft_i2c_eeprom_write_byte(uint8_t deviceAddr, uint16_t writeAddress, byte writeByte) {
//  
//  // Issue a start condition, send device address and write direction bit
//  if (!SoftI2cMasterStart((deviceAddr<<1) | I2C_WRITE)) return false;
// 
//  if (!SoftI2cMasterWrite((writeAddress >> 8))) return false; // MSB
//  if (!SoftI2cMasterWrite((writeAddress & 0xFF))) return false; // LSB
// 
//  // Write the byte
//  if (!SoftI2cMasterWrite(writeByte)) return false;
// 
//  // Issue a stop condition
//  SoftI2cMasterStop();
//  
//  return true;
// }

bool soft_i2c_eeprom_write_bytes(uint8_t deviceAddr, uint16_t writeAddress, uchar *data, uint16_t index, uchar length) {
  uchar i;
  
	// Issue a start condition, send device address and write direction bit
	if (!SoftI2cMasterStart((deviceAddr<<1) | I2C_WRITE)) return false;

	if (!SoftI2cMasterWrite((writeAddress >> 8))) return false; // MSB
	if (!SoftI2cMasterWrite((writeAddress & 0xFF))) return false; // LSB
	
	for (i = 0; i < length; i++) {
    SoftI2cMasterWrite(data[i]);
	}

	// Issue a stop condition
	SoftI2cMasterStop();
	
	return true;
}

bool soft_i2c_eeprom_erase(uint8_t deviceAddr, uint16_t writeAddress, uchar length) {
  uint16_t i;
  
	// Issue a start condition, send device address and write direction bit
	if (!SoftI2cMasterStart((deviceAddr<<1) | I2C_WRITE)) return false;

	if (!SoftI2cMasterWrite((writeAddress >> 8))) return false; // MSB
	if (!SoftI2cMasterWrite((writeAddress & 0xFF))) return false; // LSB
	
	for (i = 0; i < length; i++) {
    SoftI2cMasterWrite(65 + i);
    something++;
	}

	// Issue a stop condition
	SoftI2cMasterStop();
	
	return true;
}

// Read more than 1 byte from a device
// (Optional)
bool soft_i2c_eeprom_read_bytes(uint8_t deviceAddr, uint16_t readAddress, byte *readBuffer, uint8_t bytestoRead) {
  uint8_t i;
  
	// Issue a start condition, send device address and write direction bit
	if (!SoftI2cMasterStart((deviceAddr<<1) | I2C_WRITE)) return false;

	// Send the address to read
	if (!SoftI2cMasterWrite((readAddress >> 8))) return false; // MSB
	if (!SoftI2cMasterWrite((readAddress & 0xFF))) return false; // LSB

	// Issue a repeated start condition, send device address and read direction bit
	if (!SoftI2cMasterRestart((deviceAddr<<1) | I2C_READ)) return false;

	// Read data from the device
	for (i = 0; i < bytestoRead; i++) {
		// Send Ack until last byte then send Ack
		readBuffer[i] = SoftI2cMasterRead(i == (bytestoRead-1));
	}

	// Issue a stop condition
	SoftI2cMasterStop();
	
	return true;
}