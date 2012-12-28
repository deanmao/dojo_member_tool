ARDUINO_DIR   = /Applications/Arduino.app/Contents/Resources/Java
ARDMK_DIR     = /usr/local
BOARD_TAG    = t85
VARIANT      = tiny8
MCU          = attiny85
F_CPU        = 8000000L
ARDUINO_LIBS = SoftwareSerial EEPROM
AVRDUDE_ARD_PROGRAMMER = usbtiny

ISP_HIGH_FUSE      = 0xdf
ISP_LOW_FUSE       = 0x62
ISP_EXT_FUSE       = 0x01

include ./Arduino.mk

AVRDUDE_ARD_OPTS = -c $(AVRDUDE_ARD_PROGRAMMER)

