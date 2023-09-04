#include <Arduino.h>
#include "FastLED.h"
#include "Easing.h"

FASTLED_USING_NAMESPACE

// LED beam options
#define BEAM_DATA_PIN 10
#define BEAM_LED_TYPE WS2812B
#define BEAM_COLOR_ORDER GRB
#define BEAM_NUM_LEDS 120
#define BEAM_BRIGHTNESS 40

// LED drum options
#define DRUM_DATA_PIN 10
#define DRUM_LED_TYPE WS2812B
#define DRUM_COLOR_ORDER GRB
#define DRUM_NUM_LEDS 120
#define DRUM_BRIGHTNESS 40

// LED options
#define FRAMES_PER_SECOND 40

// Define arrays that hold led strip colour.
CRGB ledsRGB[BEAM_NUM_LEDS];
CHSV ledsHSV[BEAM_NUM_LEDS];
int trig[BEAM_NUM_LEDS + 1];

// Define easing function
EasingFunc<Ease::QuadInOut> e;
float start;

// define pins
const int buttonPin = 14; // the number of the pushbutton pin
const int ledPin = 13;    // the number of the onboard LED

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
    for (int i = 0; i < BEAM_BRIGHTNESS; i++)
    {
        ledsRGB[i] = ledsHSV[i];
    }
}

void beamAnimationRainbowComets()
{
    shiftToRight(trig, 120);
    for (int i = 0; i < BEAM_BRIGHTNESS; i++)
    {
        if (trig[i] == 1)
        {
            ledsHSV[i].v = 200;
            ledsHSV[i].h += 40;
            ledsHSV[i].s = 0;
        }
        ledsHSV[i].v = max(int(ledsHSV[i].v - random(2) * 20), 0);
        ledsHSV[i].s = min(ledsHSV[i].s + 50, 255);
    }
}

void drumAnimation()
{
}

void updateBeamAnimation()
{
    // TODO: add more animations
    beamAnimationRainbowComets();
}

void updateDrumAnimation()
{
    drumAnimation();
}

// -- MAIN LOOP --

void setup()
{
    delay(3000);

    // tell FastLED about the LED strip configuration
    FastLED.addLeds<BEAM_LED_TYPE, BEAM_DATA_PIN, BEAM_COLOR_ORDER>(ledsRGB, BEAM_NUM_LEDS).setCorrection(TypicalLEDStrip);

    // set master BEAM_BRIGHTNESS control
    FastLED.setBrightness(BEAM_BRIGHTNESS);

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
        // run drum hit animation
        updateDrumAnimation();

        // Set first element of the trigger array to 1. This will trigger the animation.
        trig[0] = 1;

        // Set the flag to true, so drum can only retrigger after being released.
        buttonFlag = true;

        // turn on onboard led
        digitalWrite(ledPin, HIGH);
    }
    else if (buttonState = LOW && buttonFlag == true)
    {
        buttonFlag = false;
        digitalWrite(ledPin, LOW);
    }

    updateBeamAnimation();
    hsv2rgb();
    FastLED.show();
    FastLED.delay(1000 / FRAMES_PER_SECOND);
}