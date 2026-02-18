/*
 * Arduino Mini - Addressable RGB LED Controller
 * Receives commands from Mach4 CNC via 3 digital inputs
 * Controls WS2812B/NeoPixel LED strips
 * 
 * Wiring:
 * - Arduino Pin 2 -> Mach4 Output Bit 0 (LSB)
 * - Arduino Pin 3 -> Mach4 Output Bit 1
 * - Arduino Pin 4 -> Mach4 Output Bit 2 (MSB)
 * - Arduino Pin 6 -> LED Strip Data In
 * - Arduino GND -> Mach4 GND (IMPORTANT!)
 * - Arduino 5V -> LED Strip VCC (or external 5V supply for longer strips)
 * - Arduino GND -> LED Strip GND (and external supply GND if used)
 */

#include <FastLED.h>

// LED Configuration
#define LED_PIN     6
#define NUM_LEDS    144
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS  20

// Input pins from Mach4 (3-bit binary)
#define INPUT_BIT0  2  // LSB
#define INPUT_BIT1  3
#define INPUT_BIT2  4  // MSB

// LED Modes (matching Mach4 states)
#define MODE_OFF      0  // 000
#define MODE_IDLE     1  // 001
#define MODE_RUNNING  2  // 010
#define MODE_PAUSED   3  // 011
#define MODE_ERROR    4  // 100
#define MODE_HOMING   5  // 101
#define MODE_TOOL     6  // 110
#define MODE_ESTOP    7  // 111 - EMERGENCY STOP - HIGHEST PRIORITY!

// Color definitions for each mode (R, G, B)
struct Color {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

const Color modeColors[8] = {
  {0, 0, 0},       // MODE_OFF - Black
  {0, 255, 0},     // MODE_IDLE - Green
  {0, 0, 255},     // MODE_RUNNING - Blue
  {255, 255, 0},   // MODE_PAUSED - Yellow
  {255, 0, 0},     // MODE_ERROR - Red
  {255, 165, 0},   // MODE_HOMING - Orange
  {255, 0, 255},   // MODE_TOOL - Magenta
  {255, 0, 0}      // MODE_ESTOP - Bright Red (alternates with white)
};

// Pattern for each mode
const uint8_t modePatterns[8] = {
  0,  // MODE_OFF - Solid (off)
  0,  // MODE_IDLE - Solid
  1,  // MODE_RUNNING - Chase
  2,  // MODE_PAUSED - Pulse
  3,  // MODE_ERROR - Fast blink
  1,  // MODE_HOMING - Chase
  2,  // MODE_TOOL - Pulse
  4   // MODE_ESTOP - Strobe (red/white flash) - MAXIMUM ATTENTION!
};

#if 0
typedef void solidColor(Color);
typedef void chase(Color);
typedef void pulse(Color);
typedef void blink(Color);
typedef void strobe(Color);

typedef void (*PatternFunc)(Color);
const PatternFunc patternFunctions[] = { solidColor, chase, pulse, blink, strobe };
#endif 


// Pattern types
#define PATTERN_SOLID     0
#define PATTERN_CHASE     1
#define PATTERN_PULSE     2
#define PATTERN_BLINK     3
#define PATTERN_STROBE    4  // Rapid red/white alternating flash for E-Stop

// LED array
CRGB leds[NUM_LEDS];

// Current state
uint8_t currentMode = MODE_OFF;


// Animation variables
unsigned long lastUpdate = 0;
uint8_t animationStep = 0;
uint8_t hue = 0;

void setup() {
  Serial.begin(115200);
  // Initialize input pins
  pinMode(INPUT_BIT0, INPUT_PULLUP);
  pinMode(INPUT_BIT1, INPUT_PULLUP);
  pinMode(INPUT_BIT2, INPUT_PULLUP);
  
  // Initialize LED strip
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  
  // Clear LEDs
  FastLED.clear();
  FastLED.show();
  
  // Startup animation
  startupAnimation();

  
}

void loop() {
  // Read the 3-bit input from Mach4
  readInputs();
  
  // Update LED animation
  updateLEDs();
  
}

// Read 3-bit binary input and decode to mode
void readInputs() {
  uint8_t bit0 = digitalRead(INPUT_BIT0) ? 1 : 0;
  uint8_t bit1 = digitalRead(INPUT_BIT1) ? 1 : 0;
  uint8_t bit2 = digitalRead(INPUT_BIT2) ? 1 : 0;


  uint8_t newMode = bit0 | bit1 << 1 | bit2 << 2;

  // Update mode if changed
  if (newMode != currentMode) {

    currentMode = newMode;
    animationStep = 0;  // Reset animation when mode changes
  }
}

void updateLEDs() {
  unsigned long currentTime = millis();
  
  // Get color and pattern for current mode
  Color color = modeColors[currentMode];
  uint8_t pattern = modePatterns[currentMode];
  
  // Update at appropriate rate based on pattern
  uint16_t updateInterval = 50; // Default 50ms
  
  switch (pattern) {
    case PATTERN_SOLID:
      updateInterval = 1000; // Slow update for solid color
      break;
    case PATTERN_CHASE:
      updateInterval = 50;
      break;
    case PATTERN_PULSE:
      updateInterval = 20;
      break;
    case PATTERN_BLINK:
      updateInterval = 250;
      break;
    case PATTERN_STROBE:
      updateInterval = 150;  // Fast strobe for E-Stop - demands attention!
      break;
  }
  
  if (currentTime - lastUpdate < updateInterval) {
    return;
  }
  lastUpdate = currentTime;
  
  // Apply pattern with current mode's color
  switch (pattern) {
    case PATTERN_SOLID:
      solidColor(color);
      break;
    case PATTERN_CHASE:
      chase(color);
      break;
    case PATTERN_PULSE:
      pulse(color);
      break;
    case PATTERN_BLINK:
      blink(color);
      break;
    case PATTERN_STROBE:
      strobe(color);  // E-Stop warning strobe
      break;
    default:
      solidColor(color);
      break;
  }
  
  FastLED.show();
  animationStep++;
}

void solidColor(Color color) {
  CRGB ledColor = CRGB(color.r, color.g, color.b);
  fill_solid(leds, NUM_LEDS, ledColor);
}

void chase(Color color) {
  fadeToBlackBy(leds, NUM_LEDS, 64);
  
  uint8_t pos = animationStep % NUM_LEDS;
 /*
  leds[pos] = CRGB(color.r, color.g, color.b);
  leds[pos] = CRGB(color.r, color.g, color.b);
 */
  leds[(pos - 1 + NUM_LEDS) % NUM_LEDS] = CRGB(color.r/2, color.g/2, color.b/2);
  leds[(pos - 2 + NUM_LEDS) % NUM_LEDS] = CRGB(color.r/4, color.g/4, color.b/4);
  // Add trailing LEDs
  if (pos > 0) leds[pos - 1] = CRGB(color.r / 2, color.g / 2, color.b / 2);
  if (pos > 1) leds[pos - 2] = CRGB(color.r / 4, color.g / 4, color.b / 4);
}

void pulse(Color color) {
  uint8_t brightness = beatsin8(60, 50, 255); // 60 BPM pulse
  
  CRGB ledColor = CRGB(color.r, color.g, color.b);
  fill_solid(leds, NUM_LEDS, ledColor);
  
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(brightness);
  }
}

void blink(Color color) {
  // Blink on/off
  if ((animationStep % 2) == 0) {
    CRGB ledColor = CRGB(color.r, color.g, color.b);
    fill_solid(leds, NUM_LEDS, ledColor);
  } else 
    fill_solid(leds, NUM_LEDS, CRGB::Black);

}

void strobe(Color color) {
  // E-Stop strobe: Alternates between bright red and bright white
  // Creates a highly visible warning pattern
  if ((animationStep % 2) == 0) {
    // Red flash
    fill_solid(leds, NUM_LEDS, CRGB(255, 0, 0));
  } else {
    // White flash
    fill_solid(leds, NUM_LEDS, CRGB(255, 255, 255));
  }
}

void startupAnimation() {
  // Quick startup flash to show system is ready
  for (int brightness = 0; brightness < 255; brightness += 5) {
    fill_solid(leds, NUM_LEDS, CRGB(0, brightness, 0));
    FastLED.show();
    delay(5);
  }
  
  for (int brightness = 255; brightness > 0; brightness -= 5) {
    fill_solid(leds, NUM_LEDS, CRGB(0, brightness, 0));
    FastLED.show();
    delay(5);
  }
  
  FastLED.clear();
  FastLED.show();
}
