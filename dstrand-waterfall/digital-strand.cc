#ifdef ARDUINO
#  include "LPD8806.h"
#  include "SPI.h" // Comment out this line if using Trinket or Gemma
#  ifdef __AVR_ATtiny85__
#    include <avr/power.h>
#  endif
#else
#  include <unistd.h>
#  include <curses.h>
#  include <iostream>
#endif

#include <stdint.h>

typedef uint8_t byte;

// Example to control LPD8806-based RGB LED Modules in a strip

/*****************************************************************************/

// Number of RGB LEDs in strand:
#define nLEDS 22
#define nSTRIPS 8
#define kSwapBlueGreen true
#define kStripScale 1


// Chose 2 pins for output; can be any valid output pins:
int dataPin  = 2;
int clockPin = 3;


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

byte SetStep(byte target, byte last) {
  byte step = (target - last) / 24;
  if (step == 0) {
    if (target > last) {
      step = 1;
    } else {
      step = -1;
    }
  }
  return step;
}

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

class ColorSeq {
public:
  ColorSeq():
    max_colors_(0) {};

  virtual ColorTuple GetNextColor(uint32_t index) {
    return colors_[index % max_colors_];
  };

protected:
  ColorTuple colors_[10];
  byte max_colors_;
};


class BlueSeq : public ColorSeq {
public:
  BlueSeq(): ColorSeq() {
    colors_[0] = ColorTuple(32, 32, 32);
    colors_[1] = ColorTuple( 0,  0, 64);
    colors_[2] = ColorTuple(19,  0, 32);
    colors_[3] = ColorTuple(36,  0, 64);

    max_colors_ = 4;
  };
};


class RainbowSeq: public ColorSeq {
public:
  RainbowSeq(): ColorSeq() {
    colors_[0] = ColorTuple(64,  0,  0);
    colors_[1] = ColorTuple(64, 32,  0);
    colors_[2] = ColorTuple(64, 64,  0);
    colors_[3] = ColorTuple(0,  64,  0);
    colors_[4] = ColorTuple(32, 32, 32);
    colors_[5] = ColorTuple(0,   0, 64);
    colors_[6] = ColorTuple(19,  0, 32);
    colors_[7] = ColorTuple(36,  0, 64);

    max_colors_ = 8;
  };
};


class RedSeq: public ColorSeq {
public:
  RedSeq(): ColorSeq() {
    colors_[0] = ColorTuple(48,  0,  0);
    colors_[1] = ColorTuple(48, 16,  0);
    colors_[2] = ColorTuple(48,  0,  0);
    colors_[3] = ColorTuple(48, 20,  0);
    colors_[4] = ColorTuple(48,  0,  0);
    colors_[5] = ColorTuple(48,  8,  0);

    max_colors_ = 6;
  };
};


ColorSeq sequence = RedSeq();

class Color {
public:
  Color():
    red_(0), green_(0), blue_(0), index_(0) {
    SetTarget(index_);
  };

  Color(ColorTuple tuple):
    Color(tuple.red_, tuple.green_, tuple.blue_) {};

  Color(byte red, byte green, byte blue):
    red_(red),
    green_(green),
    blue_(blue),
    index_(0),
    step_red_(0),
    step_green_(0),
    step_blue_(0),
    target_red_(red),
    target_green_(green),
    target_blue_(blue) {
  };

  byte GetRed() {
    return red_;
  };
  byte GetGreen() {
    return green_;
  };
  byte GetBlue() {
    return blue_;
  };
  
  void StepColor() {
    SetIntermediateColor();
    if (AtTarget()) {
      PickNextColor();
    };
  }

  void SetTarget(uint32_t index) {
    ColorTuple next = sequence.GetNextColor(index);

    target_red_ = next.red_;
    target_green_ = next.green_;
    target_blue_ = next.blue_;

    step_red_ = SetStep(target_red_, red_);
    step_green_ = SetStep(target_green_, green_);
    step_blue_ = SetStep(target_blue_, blue_);

    index_ = index;
  };

private:
  void  SetIntermediateColor() {
    red_ = MoveToTarget(red_, target_red_, step_red_);
    green_ = MoveToTarget(green_, target_green_, step_green_);
    blue_ = MoveToTarget(blue_, target_blue_, step_blue_);
  };

  void PickNextColor() {
    SetTarget(index_ + 1);
  }

  bool AtTarget() {
    return (red_ == target_red_
            && green_ == target_green_
            && blue_ == target_blue_);
  };

  byte red_;
  byte green_;
  byte blue_;
  uint32_t index_;
  byte step_red_;
  byte step_green_;
  byte step_blue_;
  byte target_red_;
  byte target_green_;
  byte target_blue_;
};


class Strip {
public:
  Strip(byte size):
  size_(size) {
    for (int i = 0; i < size; i++) {
      pixels_[i] = Color(sequence.GetNextColor(i));
      pixels_[i].SetTarget(i + 1);
    }
  };

  byte numPixels() {
    return size_;
  };

  void setPixelColor(const byte& pixel, const class Color& color) {
    pixels_[pixel] = color;
  };

  void StepColor(const byte& pixel) {
    pixels_[pixel].StepColor();
  }


  virtual void begin() = 0;
  virtual void show() = 0;

protected:
  byte size_;
  class Color pixels_[nLEDS];

};

#ifdef ARDUINO
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

#else

class NcursesStrip: public Strip {
public:
  NcursesStrip(byte size):
    Strip(size)
  {};

  virtual void begin() {};

  virtual void show() {
    for (unsigned int i = 0; i < size_; i++) {
      SetPixelColor(i);
      
    }
    wrefresh(stdscr);
  };
private:
  void SetPixelColor(short pixel) {
    unsigned int pair = pixel + 32; // static offset.
    
    byte red = pixels_[pixel].GetRed();
    byte green = pixels_[pixel].GetGreen();
    byte blue = pixels_[pixel].GetBlue();
    
    // Create colors starting at 32 (== pair):
    init_color(pair, red * 10, green * 10, blue * 10);

    // Assign it to a color pair:
    init_pair(pair, pair, COLOR_BLACK);

    // Write a character:
    mvwaddch(stdscr, 5, 10 + pixel, '#' | COLOR_PAIR(pair));
  };
};
#endif

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
# ifdef ARDUINO
  return new ArduinoStrip(num_leds);
# else
  return new NcursesStrip(num_leds);
# endif
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

ColorSeq color_seq_seq[3] = {BlueSeq(), RedSeq(), RainbowSeq()};
const uint32_t kIterationThreshold = 10000;
const byte kColorSeqLen = 3;

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

#ifndef ARDUINO
int main(void) {
  initscr();
  start_color();
  if (!can_change_color()) {
    printw("Can't change colors!\n");
    puts("Can't change colors!");
    endwin();
    return 1;
  }
  use_default_colors();
  init_pair(128, COLOR_WHITE, COLOR_BLACK);
  bkgd(COLOR_PAIR(128));
  printw("max colors: %d\n", COLORS);
  printw("max pairs: %d\n", COLOR_PAIRS);
  usleep(1000 * 1000);
  setup();

  int count = 0;
  do {
    loop();
    usleep(1000);
  } while (count++ < 10000); //true); // count++ < 1000);
  endwin();
  printf("max colors: %d\n", COLORS);
  printf("max pairs: %d\n", COLOR_PAIRS);
  return 0;
}
#endif
