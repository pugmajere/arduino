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
#define nLEDS 32

int nLEDs = 32;

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

// The sequence of colors the LEDs move through.
enum kColor_Names {RED, GREEN, BLUE};
uint32_t color_index = 0;
uint32_t colors[][3] = {{64,  0,  0},
                        {64, 32,  0},
                        {64, 64,  0},
                        {0,  64,  0},
                        {0,   0, 64},
                        {19,  0, 32},
                        {36,  0, 64},
};
const uint32_t kNumColors = 7;

class Color {
public:
  Color():
    red_(0), green_(0), blue_(0), index_(0) {
    SetTarget(index_);
  };

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
    target_red_ = colors[index][RED];
    target_green_ = colors[index][GREEN];
    target_blue_ = colors[index][BLUE];

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
    SetTarget((index_ + 1) % kNumColors);
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
      pixels_[i] = Color(colors[i % kNumColors][RED],
                         colors[i % kNumColors][BLUE],
                         colors[i % kNumColors][GREEN]);
      pixels_[i].SetTarget((i + 1) % kNumColors);
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
    strip_ = LPD8806(nLEDs, dataPin, clockPin);
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
    strip_.setPixelColor(pixel, strip_.Color(pixels_[pixel].GetRed(),
                                             pixels_[pixel].GetGreen(),
                                             pixels_[pixel].GetBlue()));
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
unsigned long strip_interval = 10; // Time between LED activations.
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


void loop() {
  // check to see if it's time to blink the LED; that is, if the 
  // difference between the current time and last time you blinked 
  // the LED is bigger than the interval at which you want to 
  // blink the LED.
  unsigned long currentMillis = millis();
 
  // Adjust the light strip.
  if (currentMillis - strip_last_change_millis > strip_interval) {
    strip_last_change_millis = currentMillis;
    mystrip->StepColor(pixel);
    mystrip->show();

    pixel += 1;
    if (pixel == mystrip->numPixels()) {
      pixel = 0;
    }
  }
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
    usleep(10 * 1000);
    count++;
  } while (count++ < 10000); //true); // count++ < 1000);
  endwin();
  printf("max colors: %d\n", COLORS);
  printf("max pairs: %d\n", COLOR_PAIRS);
  return 0;
}
#endif
