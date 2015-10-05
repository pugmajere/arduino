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
long strip_interval = 10; // Time between LED activations.
long strip_last_change_millis = 0;
uint32_t pixel = 0; // Pixel to act on.
uint32_t color = 0;
uint32_t target_color = 0;

const byte kColor_min = 0;
const byte kColor_max = 64;


void pickRandomColor() {
  byte r, g, b;
  r = random(kColor_min, kColor_max);
  g = random(kColor_min, kColor_max);
  b = random(kColor_min, kColor_max);
  target_color = strip.Color(r, g, b);
}

byte last_r, last_g, last_b = 0;
byte current_r, current_g, current_b = 0;
byte target_r, target_g, target_b = 0;
byte step_r, step_g, step_b = 0;

byte SetStep(byte target, byte last) {
  byte step = (target - last) / 16;
  if (step == 0) {
    if (target > last) {
      step = 1;
    } else {
      step = -1;
    }
  }
  return step;
}

uint32_t color_index = 0;

// std::vector<std::tuple<byte, byte, byte> > colors;

void pickNextColor() {
  last_r = current_r;
  last_g = current_g;
  last_b = current_b;
  
  if (last_r == kColor_max) {
    target_r = kColor_min;
    target_g = kColor_max;
    target_b = kColor_min;
  } else if (last_g == kColor_max) {
    target_r = kColor_min;
    target_g = kColor_min;
    target_b = kColor_max;
  } else if (last_b == kColor_max) {
    target_r = kColor_max;
    target_g = kColor_min;
    target_b = kColor_min;
  } else {
    target_r = kColor_max;
  }

  step_r = SetStep(target_r, last_r);
  step_g = SetStep(target_g, last_g);
  step_b = SetStep(target_b, last_b);
}

byte MoveToTarget(byte current, byte target, byte step) {
  bool greater = (current > target);
  byte future = (current + step);

  if (greater) {
    if (future > current || current < target) {
      future = target;
    }
  } else {
    if (future < current || future > target) {
      future = target;
    }
  }
  return future;
}

uint32_t GetIntermediateColor(uint32_t target, uint32_t current) {
  current_r = MoveToTarget(current_r, target_r, step_r);
  current_g = MoveToTarget(current_g, target_g, step_g);
  current_b = MoveToTarget(current_b, target_b, step_b);

  return strip.Color(current_r, current_g, current_b);
}

bool CheckAtTarget() {
  return (current_r == target_r
          && current_g == target_g
          && current_b == target_b);
}
 

void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif

  // set the digital pin as output:
  pinMode(ledPin, OUTPUT);

  // Start up the LED strip
  strip.begin();
  // Update the strip, to start they are all 'off'
  strip.show();

  randomSeed(0);
  pickNextColor();
  // pickRandomColor();

  for (pixel = 0; pixel < strip.numPixels(); pixel++) {
    strip.setPixelColor(pixel, target_color);
  }
  strip.show();

  color = target_color;
  // pickRandomColor();
  pickNextColor();
  color = GetIntermediateColor(target_color, color);
  
  pixel = 0;
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

    pixel += 1;
    if (pixel == strip.numPixels()) {
      pixel = 0;
      uint32_t this_color = strip.getPixelColor(pixel);
      color = GetIntermediateColor(target_color, this_color);

      if (CheckAtTarget()) {
        pickNextColor();
        // pickRandomColor();
      }
    }
  }
}

