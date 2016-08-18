BOARD_TAG    = uno
ARDUINO_LIBS = Adafruit_NeoPixel LPD8806 SPI
ARDUINO_SKETCHBOOK = /home/ryan/Arduino

include /usr/share/arduino/Arduino.mk

CHK_SOURCES = digital-strand.cc

.PHONY: check-syntax
check-syntax:
	g++ -std=gnu++11 -Wall -Wextra -pedantic -fsyntax-only $(CHK_SOURCES)
