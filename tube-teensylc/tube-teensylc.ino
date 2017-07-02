#include "LPD8806.h"
#include "SPI.h" // Comment out this line if using Trinket or Gemma
#ifdef __AVR__
  #include <avr/power.h>
#endif

#include <stdint.h>

typedef uint8_t byte;

// Example to control LPD8806-based RGB LED Modules in a strip

/*****************************************************************************/

// Number of RGB LEDs in strand:
#define nLEDS 22
#define nSTRIPS 8
#define kSwapBlueGreen true
#define kStripScale 0.5


// Chose 2 pins for output; can be any valid output pins:
int dataPin  = 11;
int clockPin = 13;


class ColorTuple {
public:
  ColorTuple(byte red, byte green, byte blue):
    red_(red), green_(green), blue_(blue) {
  };
  ColorTuple(): ColorTuple(0, 0, 0) {};

  byte red_;
  byte green_;
  byte blue_;
};

class Color {
public:
  Color():
    red_(0), green_(0), blue_(0) {
  };

  Color(ColorTuple tuple):
    Color(tuple.red_, tuple.green_, tuple.blue_) {};

  Color(byte red, byte green, byte blue):
    red_(red),
    green_(green),
    blue_(blue) {  };

  byte GetRed() {
    return red_;
  };
  byte GetGreen() {
    return green_;
  };
  byte GetBlue() {
    return blue_;
  };

private:

  byte red_;
  byte green_;
  byte blue_;
};


class Strip {
public:
  Strip(byte size):
    size_(size) {};

  byte numPixels() {
    return size_;
  };

  void setPixelColor(const byte& pixel, const class Color& color) {
    pixels_[pixel] = color;
  };

  virtual void begin() = 0;
  virtual void show() = 0;

protected:
  byte size_;
  class Color pixels_[nLEDS];

};

class ArduinoStrip: public Strip {
public:
  ArduinoStrip(byte size):
    Strip(size) {
    strip_ = LPD8806(nLEDS * nSTRIPS, dataPin, clockPin);

  };

  virtual void begin() {
    strip_.begin();
  };

  virtual void show() {
    for (unsigned int i = 0; i < size_; i++) {
      SetPixelColor(i);
    }
    strip_.show();
  };

private:
  void SetPixelColor(short pixel) {
    for (int i = 0; i < nSTRIPS; i++) {
      byte red = pixels_[pixel].GetRed() * kStripScale;
      byte blue = pixels_[pixel].GetBlue() * kStripScale;
      byte green = pixels_[pixel].GetGreen() * kStripScale;

      if (kSwapBlueGreen) {
        strip_.setPixelColor(pixel + (nLEDS * i),
                             strip_.Color(red, blue, green));
      } else {
        strip_.setPixelColor(pixel + (nLEDS * i),
                             strip_.Color(red, green, blue));
      }
    }
  };

  // First parameter is the number of LEDs in the strand.  The LED strips
  // are 32 LEDs per meter but you can extend or cut the strip.  Next two
  // parameters are SPI data and clock pins:
  LPD8806 strip_;
  
};

Strip *mystrip = NULL;

// Variables will change:
unsigned long previousMillis = 0;        // will store last time LED was updated

// the follow variables is a long because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long interval = 1000;           // interval at which to blink (milliseconds)

long min_interval = 50;
long max_interval = 1000;
bool interval_decreasing = true;
long interval_factor = 2;

// State for the strip:
unsigned long strip_interval = 3; // Time between LED activations.
unsigned long strip_last_change_millis = 0;
uint32_t pixel = 0; // Pixel to act on.

Color color(0, 0, 0); // Active color.
uint32_t target_color = 0;

byte last_r, last_g, last_b = 0;
byte current_r, current_g, current_b = 0;
byte target_r, target_g, target_b = 0;
byte step_r, step_g, step_b = 0;

Strip* CreateStrip(byte num_leds) {
  return new ArduinoStrip(num_leds);
}

void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif
  
  mystrip = CreateStrip(nLEDS);

  // Start up the LED strip
  mystrip->begin();

  // Update the strip, to start they are all 'off'
  mystrip->show();

  pixel = 0;
}

#ifndef ARDUINO
long millis() {
  static long now = 0;
  long ret = now;
  now += strip_interval;
  return ret;
}
#endif

uint32_t iterations = 0;

//Input a value 0 to 384 to get a color value.
//The colours are a transition r - g -b - back to r

Color Wheel(uint16_t WheelPos)
{
  byte r, g, b;
  switch(WheelPos / 128)
  {
    case 0:
      r = 127 - WheelPos % 128;   //Red down
      g = WheelPos % 128;      // Green up
      b = 0;                  //blue off
      break; 
    case 1:
      g = 127 - WheelPos % 128;  //green down
      b = WheelPos % 128;      //blue up
      r = 0;                  //red off
      break; 
    case 2:
      b = 127 - WheelPos % 128;  //blue down 
      r = WheelPos % 128;      //red up
      g = 0;                  //green off
      break; 
  }
  return(Color(ColorTuple(r,g,b)));
}

// Slightly different, this one makes the rainbow wheel equally distributed 
// along the chain
void rainbowCycle() {
  uint16_t i, j;
  for (j=0; j < 384; j++) {     // 5 cycles of all 384 colors in the wheel

    for (i=0; i < mystrip->numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 384-color wheel
      // (thats the i / strip.numPixels() part)
      // Then add in j which makes the colors go around per pixel
      // the % 384 is to make the wheel cycle around
      mystrip->setPixelColor(i, Wheel( ((i * 384 / mystrip->numPixels()) + j) % 384) );
    }  
    mystrip->show();   // write all the pixels out

    delay(1);
  }
}

Color RedYellowWheel(uint16_t WheelPos) {
  byte r, g, b;
  switch(WheelPos / 64)
  {
    case 0:
      r = 127 - WheelPos % 64;   //Red down
      g = (WheelPos % 64) / 3;      // Green up
      b = 0;                  //blue off
      break; 
    case 1:
      g = (64 - WheelPos % 64) / 3;  //green down
      r = 64 + WheelPos % 64;      //red up
      b = 0;                  //blue off
      break; 
  }
  return(Color(ColorTuple(r,g,b)));
}

void redYellowCycle() {
  uint16_t i, j;
  
  for (j=0; j < 128; j++) {
    for (i=0; i < mystrip->numPixels(); i++) {
      mystrip->setPixelColor(i, RedYellowWheel(((i * 128 / mystrip->numPixels()) + j) % 128));
    }
    mystrip->show();

    delay(4);
  }
}

Color BlueWheel(uint16_t WheelPos) {
  byte r, g, b;
  switch(WheelPos / 64)
  {
    case 0:
      b = 96 - WheelPos % 64;   //blue down
      r = (WheelPos % 64) / 2;      // red up
      g = 0;                  //blue off
      break; 
    case 1:
      r = (64 - WheelPos % 64) / 2;  //red down
      b = 32 + WheelPos % 64;      //blue up
      g = 0;                  //green off
      break; 
  }
  return(Color(ColorTuple(r,g,b)));
}

void blueCycle() {
  uint16_t i, j;
  
  for (j=0; j < 128; j++) {
    for (i=0; i < mystrip->numPixels(); i++) {
      mystrip->setPixelColor(i, BlueWheel(((i * 128 / mystrip->numPixels()) + j) % 128));
    }
    mystrip->show();

    delay(4);
  }
}


void loop() {
  switch((iterations / 30) % 3) {
  case 0:
    rainbowCycle();
    break;
  case 1:
    // Runs in 1/3rd the time, so run it 3 times.
    redYellowCycle();
    redYellowCycle();
    redYellowCycle();
    break;
  case 2:
    // Runs in 1/3rd the time, so run it 3 times.
    blueCycle();
    blueCycle();
    blueCycle();
    break;
  }

  iterations++;
}
