#include <Arduino.h>
#include "FastLED.h"
#include "Easing.h"

FASTLED_USING_NAMESPACE

#define DATA_PIN 10
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS 120

#define BRIGHTNESS 40
#define FRAMES_PER_SECOND 40

CRGB ledsRGB[NUM_LEDS];
CHSV ledsHSV[NUM_LEDS];
int trig[NUM_LEDS + 1];

EasingFunc<Ease::QuadInOut> e;
float start;

// define pins
const int buttonPin = 14; // the number of the pushbutton pin
const int ledPin = 13;    // the number of the LED pin

// define variable and flag to store the button state
int buttonState = 0;
bool buttonFlag = false;

// define animation variables
uint8_t currentAnimation = 0; // Index number of which pattern is current
uint8_t masterHue = 0;        // rotating "base color" used by many of the patterns

// shift all elements upto the n-th position 1 position to the right
void shiftToRight(int a[], int n)
{
    for (int i = n; i > 0; i--)
    {
        a[i] = a[i - 1];
    }

    a[0] = 0;
}

// convert all HSV values to RGB values
void hsv2rgb()
{
    for (int i = 0; i < NUM_LEDS; i++)
    {
        ledsRGB[i] = ledsHSV[i];
    }
}

void animationRainbowComets()
{
    shiftToRight(trig, 120); // TODO: why 61?
    for (int i = 0; i < NUM_LEDS; i++)
    {
        if (trig[i] == 1)
        {
            ledsHSV[i].v = 200;
            ledsHSV[i].h += 40;
            ledsHSV[i].s = 0;
        }
        ledsHSV[i].v = max(ledsHSV[i].v - random(2) * 20, 0);
        ledsHSV[i].s = min(ledsHSV[i].s + 50, 255);
    }
}

void updateAnimationFrame()
{
    // TODO: add more animations
    animationRainbowComets();
}

// -- MAIN LOOP --

void setup()
{
    delay(3000);

    // tell FastLED about the LED strip configuration
    FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(ledsRGB, NUM_LEDS).setCorrection(TypicalLEDStrip);

    // set master brightness control
    FastLED.setBrightness(BRIGHTNESS);

    // initialize serial communication at 9600 bits per second:
    Serial.begin(9600);

    // initialize the LED pin as an output:
    pinMode(ledPin, OUTPUT);

    // initialize the buttonpin as an input:
    pinMode(buttonPin, INPUT);
}

void loop()
{

    // read the state of the pushbutton value:
    buttonState = digitalRead(buttonPin);

    // is buttonpin is high, buttonState is HIGH:
    if (buttonState == HIGH && buttonFlag == false)
    {
        trig[0] = 1;
        buttonFlag = true;
        digitalWrite(ledPin, HIGH);
    }
    else if (buttonState = LOW && buttonFlag == true)
    {
        buttonFlag = false;
        digitalWrite(ledPin, LOW);
    }

    updateAnimationFrame();
    hsv2rgb();
    FastLED.show();
    FastLED.delay(1000 / FRAMES_PER_SECOND);
}