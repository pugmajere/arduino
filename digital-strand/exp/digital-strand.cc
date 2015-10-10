#include <unistd.h>
#include <stdint.h>
#include <vector>
#include <curses.h>
#include <iostream>

typedef uint8_t byte;

// Example to control LPD8806-based RGB LED Modules in a strip

/*****************************************************************************/

// Number of RGB LEDs in strand:
int nLEDs = 32;

// Chose 2 pins for output; can be any valid output pins:
int dataPin  = 2;
int clockPin = 3;

class Color {
public:
  Color():
    red_(0), green_(0), blue_(0) {};

  Color(byte red, byte green, byte blue):
    red_(red),
    green_(green),
    blue_(blue) {
  };

  void SetColorPair(short colornum) {
    if (red_ | green_ | blue_)
      printw("%03d %03d %03d", red_, green_, blue_);
    init_color(colornum, red_, green_, blue_);
    init_pair(colornum, colornum, 0);
  }

private:
  byte red_;
  byte green_;
  byte blue_;
};


class Strip {
public:
  Strip(byte size):
  size_(size), pixels_(size) {};

  static uint32_t Color(byte r, byte g, byte b) {
    return Color(r, g, b);
  };

  byte numPixels() {
    return size_;
  };

  void setPixelColor(const byte& pixel, const class Color& color) {
    pixels_[pixel] = color;
  };

  void begin() {};
  void show() {
    for (unsigned int i = 0; i < pixels_.size(); i++) {
      unsigned int pair = i + 32; // static offset.
      pixels_[i].SetColorPair(pair);
      mvwaddch(stdscr, 5, 10 + i, int('#' + i) | COLOR_PAIR(pair));
      
    }
    wrefresh(stdscr);
    
  };

private:
  byte size_;
  std::vector<class Color> pixels_;

};
Strip strip(nLEDs);

enum State {LOW, HIGH};
enum PinState {INPUT, OUTPUT};

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

const byte kColor_min = 0;
const byte kColor_max = 64;

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
uint32_t colors[][3] = {{0, 0, 0},
                        {64, 0, 0},
                        {48, 16, 0},
                        {0, 0, 64}};

void pickNextColor() {
  last_r = current_r;
  last_g = current_g;
  last_b = current_b;

  color_index = (color_index + 1) % 4;

  target_r = colors[color_index][0];
  target_g = colors[color_index][1];
  target_b = colors[color_index][2];

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

Color GetIntermediateColor() {
  current_r = MoveToTarget(current_r, target_r, step_r);
  current_g = MoveToTarget(current_g, target_g, step_g);
  current_b = MoveToTarget(current_b, target_b, step_b);

  return Color(current_r, current_g, current_b);
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

  // Start up the LED strip
  strip.begin();
  // Update the strip, to start they are all 'off'
  strip.show();

  pickNextColor();
  color = GetIntermediateColor();
  
  pixel = 0;
}

long millis() {
  static long now = 0;
  long ret = now;
  now += strip_interval;
  return ret;
}


void loop() {
  // check to see if it's time to blink the LED; that is, if the 
  // difference between the current time and last time you blinked 
  // the LED is bigger than the interval at which you want to 
  // blink the LED.
  unsigned long currentMillis = millis();
 
  // Adjust the light strip.
  if (currentMillis - strip_last_change_millis > strip_interval) {
    strip_last_change_millis = currentMillis;
    strip.setPixelColor(pixel, color);
    strip.show();

    pixel += 1;
    if (pixel == strip.numPixels()) {
      pixel = 0;
      color = GetIntermediateColor();

      if (CheckAtTarget()) {
        pickNextColor();
      }
    }
  }
}

int main(void) {
  initscr();
  start_color();
  std::cout << "max colors: " << COLORS << std::endl;
  std::cout << "max pairs: " << COLOR_PAIRS << std::endl;
  setup();
  do {
    loop();
    usleep(1 * 1000);
  } while (true);
}
