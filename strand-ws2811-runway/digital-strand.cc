#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 6
#define NLEDS 320

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(NLEDS, PIN, NEO_GRB + NEO_KHZ800);


// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  // End of trinket special code


  strip1.begin();

  strip1.show(); // Initialize all pixels to 'off'
}

#define DELAY_MS 75
#define LED_DIV 15

void SetColors(uint16_t pos, uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < LED_DIV; i++) {
    strip1.setPixelColor(pos + (NLEDS / LED_DIV * i), r, g, b);
  }
}

uint8_t leslie_colors[][3] = {
  {0, 0, 255}, // Blue
  {96, 0, 128},
  {0, 127, 127}, // cyan
  {64, 0, 128},
  {48, 0, 96},
};

uint8_t color_index = 0;
uint16_t last_led = NLEDS / LED_DIV;
unsigned long last_millis = 0;

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - last_millis > DELAY_MS) {
    SetColors(last_led, 0, 0, 0);

    last_led--;
    if (last_led > NLEDS) {
      last_led = NLEDS / LED_DIV - 1;
    }

    SetColors(last_led,
              leslie_colors[color_index][0],
              leslie_colors[color_index][1],
              leslie_colors[color_index][2]);
    strip1.show();
    last_millis = currentMillis;

    if (last_led == 0) {
      color_index = (color_index + 1) % (sizeof(leslie_colors) / 3);
    }
  }
}
