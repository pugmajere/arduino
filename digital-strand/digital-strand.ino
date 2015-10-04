#include "LPD8806.h"
#include "SPI.h" // Comment out this line if using Trinket or Gemma
#ifdef __AVR_ATtiny85__
 #include <avr/power.h>
#endif

// Example to control LPD8806-based RGB LED Modules in a strip

/*****************************************************************************/

// Number of RGB LEDs in strand:
int nLEDs = 32;

// Chose 2 pins for output; can be any valid output pins:
int dataPin  = 2;
int clockPin = 3;

// First parameter is the number of LEDs in the strand.  The LED strips
// are 32 LEDs per meter but you can extend or cut the strip.  Next two
// parameters are SPI data and clock pins:
LPD8806 strip = LPD8806(nLEDs, dataPin, clockPin);

// You can optionally use hardware SPI for faster writes, just leave out
// the data and clock pin parameters.  But this does limit use to very
// specific pins on the Arduino.  For "classic" Arduinos (Uno, Duemilanove,
// etc.), data = pin 11, clock = pin 13.  For Arduino Mega, data = pin 51,
// clock = pin 52.  For 32u4 Breakout Board+ and Teensy, data = pin B2,
// clock = pin B1.  For Leonardo, this can ONLY be done on the ICSP pins.
//LPD8806 strip = LPD8806(nLEDs);


// For blinking the status LED:
// constants won't change. Used here to 
// set pin numbers:
const int ledPin =  13;      // the number of the LED pin

// Variables will change:
int ledState = LOW;             // ledState used to set the LED
long previousMillis = 0;        // will store last time LED was updated

// the follow variables is a long because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long interval = 1000;           // interval at which to blink (milliseconds)

long min_interval = 50;
long max_interval = 1000;
boolean interval_decreasing = true;
long interval_factor = 2;

// State for the strip:
long strip_interval = 1000; // Time between LED activations.
long strip_last_change_millis = 0;
boolean strip_ascend = true;
uint32_t pixel = 0; // Pixel to act on.
uint32_t color = 0;

byte color_min = 0;
byte color_max = 128;


void pickRandomColor() {
  byte r, g, b;
  r = random(color_min, color_max);
  g = random(color_min, color_max);
  b = random(color_min, color_max);
  color = strip.Color(r, g, b);
}


void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif

  // Start up the LED strip
  strip.begin();

  // Update the strip, to start they are all 'off'
  strip.show();

  // set the digital pin as output:
  pinMode(ledPin, OUTPUT);

  randomSeed(0);

  pickRandomColor();

}


void loop() {
  // check to see if it's time to blink the LED; that is, if the 
  // difference between the current time and last time you blinked 
  // the LED is bigger than the interval at which you want to 
  // blink the LED.
  unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis > interval) {
    // save the last time you blinked the LED 
    previousMillis = currentMillis;   

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW)
      ledState = HIGH;
    else
      ledState = LOW;

    // set the LED with the ledState of the variable:
    digitalWrite(ledPin, ledState);

    // Adjust the interval:
    if (interval_decreasing) {
      interval /= interval_factor;
      if (interval < min_interval) {
        interval = min_interval;
        interval_decreasing = false;
      }
    } else {
      // interval_increasing
      interval *= interval_factor;
      if (interval > max_interval) {
        interval = max_interval;
        interval_decreasing = true;
      }
    }
  }

  // Adjust the light strip.
  if (currentMillis - strip_last_change_millis > strip_interval) {
    strip_last_change_millis = currentMillis;
    strip.setPixelColor(pixel, color);
    strip.show();

    switch(strip_ascend)
      {
      case false:
        pixel -= 1;
        if (pixel == 0) {
          strip_ascend = true;
          pickRandomColor();
        }
        break;
      case true:
        pixel += 1;
        if (pixel == strip.numPixels()) {
          strip_ascend = false;
          pickRandomColor();
        }
        break;
      }
  }
}

