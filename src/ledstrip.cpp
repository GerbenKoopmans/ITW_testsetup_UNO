#include <Arduino.h>
#include "FastLED.h"
#include "Easing.h"

FASTLED_USING_NAMESPACE

// FastLED "100-lines-of-code" demo reel, showing just a few
// of the kinds of animation patterns you can quickly and easily
// compose using FastLED.
//
// This example also shows one easy way to define multiple
// animations patterns and have them automatically rotate.
//
// -Mark Kriegsman, December 2014

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DATA_PIN    3
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    60
CRGB ledsRGB[NUM_LEDS];
CHSV ledsHSV[NUM_LEDS];
int trig[NUM_LEDS + 1];

EasingFunc<Ease::QuadInOut> e;
float start;

#define BRIGHTNESS          96
#define FRAMES_PER_SECOND   40

// constants won't change. They're used here to set pin numbers:
const int buttonPin = 4;     // the number of the pushbutton pin
const int ledPin =  13;      // the number of the LED pin

// variables will change:
int buttonState = 0;         // variable for reading the pushbutton status
bool buttonFlag = false;


void setup() {
  delay(3000); // 3 second delay for recovery

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(ledsRGB, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  Serial.begin(9600);
  // initialize the LED pin as an output:
  pinMode(ledPin, OUTPUT);
  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT);
}

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
int pos = 0;

void loop()
{

  // read the state of the pushbutton value:
  buttonState = digitalRead(buttonPin);

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == HIGH && buttonFlag == false) {
    trig[0] = 1;
    buttonFlag = true;
    digitalWrite(ledPin, HIGH);
  } else if(buttonState = LOW && buttonFlag == true){
    buttonFlag = false;
    digitalWrite(ledPin, LOW);
  }

  shiftToRight(trig, 61);
  sinelon();
  hsv2rgb();
  FastLED.show();
  FastLED.delay(1000 / FRAMES_PER_SECOND);
}


void sinelon()
{

  for (int i = 0; i < NUM_LEDS; i++) {
    if (trig[i] == 1) {
      ledsHSV[i].v = 200;
      ledsHSV[i].h += 40;
      ledsHSV[i].s = 0;
    }
    ledsHSV[i].v = max(ledsHSV[i].v - random(2)*20, 0);
    ledsHSV[i].s = min(ledsHSV[i].s + 50, 255);
  }
}

void shiftToRight(int a[], int n) {
  int temp = a[n];

  for (int i = n; i > 0; i--) {
    a[i] = a[i - 1];
  }

  a[0] = 0;
}

void hsv2rgb() {
  for (int i = 0; i < NUM_LEDS; i++) {
    ledsRGB[i] = ledsHSV[i];
  }
}