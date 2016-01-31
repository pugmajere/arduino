BOARD_TAG    = uno
ARDUINO_LIBS = LPD8806 SPI

include /usr/share/arduino/Arduino.mk

CHK_SOURCES = digital-strand.cc

.PHONY: check-syntax
check-syntax:
	g++ -std=gnu++11 -Wall -Wextra -pedantic -fsyntax-only $(CHK_SOURCES)
