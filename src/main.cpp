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
#define DRUM_DATA_PIN 11
#define DRUM_LED_TYPE WS2812B
#define DRUM_COLOR_ORDER GRB
#define DRUM_NUM_LEDS 60
#define DRUM_BRIGHTNESS 40

// LED options
#define FRAMES_PER_SECOND 40

// Define pins
#define TRIGGER_PIN 14
#define ONBOARD_LED_PIN 13

// Define arrays that hold led beam colour.
CRGB beamLedsRGB[BEAM_NUM_LEDS];
CHSV beamLedsHSV[BEAM_NUM_LEDS];

// Define array that holds led drum colour.
CRGB drumLedsRGB[DRUM_NUM_LEDS];
CHSV drumLedsHSV[DRUM_NUM_LEDS];

// Define array that holds trigger values for drum hit.
int trig[BEAM_NUM_LEDS + 1];

// Define easing function
EasingFunc<Ease::QuadInOut> e;
float start;

// Define variable and flag to store the button state
int triggerValue = 0;
bool triggerFlag = false;
int triggerThreshold = 100;
int triggerHysterisis = 50;

// Define animation variables
uint8_t currentAnimation = 0; // Index number of which pattern is current
uint8_t masterHue = 0;        // rotating "base color" used by many of the patterns

// Define idle timer
unsigned long idleTimer = 0;
unsigned long idleThreshold = 10000;

// Led ring idle animation
int ledPosDrum = 0;
int idleHue = 100;
int idleValue = 200;
boolean isFullyColored = true;

uint8_t status;
enum status_types_t
{
    ACTIVE,
    IDLE
};

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
    for (int i = 0; i < BEAM_NUM_LEDS; i++)
    {
        beamLedsRGB[i] = beamLedsHSV[i];
    }

    for (int i = 0; i < DRUM_NUM_LEDS; i++)
    {
        drumLedsRGB[i] = drumLedsHSV[i];
    }
}

void beamAnimationRainbowComets()
{
    shiftToRight(trig, 120);
    for (int i = 0; i < BEAM_NUM_LEDS; i++)
    {
        if (trig[i] == 1)
        {
            beamLedsHSV[i].v = 200;
            beamLedsHSV[i].h += 40;
            beamLedsHSV[i].s = 0;
        }
        beamLedsHSV[i].v = max(int(beamLedsHSV[i].v - random(2) * 20), 0);
        beamLedsHSV[i].s = min(beamLedsHSV[i].s + 50, 255);
    }
}

void drumAnimation()
{
    // if drum hit, fire up all leds
    if (trig[0] == 1)
    {
        for (int i = 0; i < DRUM_NUM_LEDS; i++)
        {
            drumLedsHSV[i].v = 200;
            drumLedsHSV[i].h += 40;
            drumLedsHSV[i].s = 0;
        }
    }

    // decrease all leds brightness
    for (int i = 0; i < DRUM_NUM_LEDS; i++)
    {
        int release = 40;

        drumLedsHSV[i].v = max(int(drumLedsHSV[i].v - release), 0);
        drumLedsHSV[i].h = min(drumLedsHSV[i].s + 50, 255);
    }
}

void drumIdleAnimation()
{
    // Give led a random color
    drumLedsHSV[ledPosDrum].v = idleValue;
    drumLedsHSV[ledPosDrum].h = idleHue;
    drumLedsHSV[ledPosDrum].s = 0;

    // Update LED position
    if (ledPosDrum <= DRUM_NUM_LEDS)
    {
        ledPosDrum++;
    }
    // Turn LEDs off
    else if (isFullyColored)
    {
        ledPosDrum = 0;
        idleValue = 0;
        isFullyColored = false;
    }
    // Turn LEDs on
    else if (!isFullyColored)
    {
        ledPosDrum = 0;
        idleValue = 200;
        idleHue = random(0, 255);
        isFullyColored = true;
    }

    Serial.println(ledPosDrum);
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

void checkForIdle()
{
    // if idle timer is greater than idleThreshold, set status to IDLE
    if (millis() - idleTimer > idleThreshold)
    {
        status = IDLE;
    }
}

void resetIdleTimer()
{
    idleTimer = millis();
    status = ACTIVE;
    Serial.println(status);
}

// have a random chance of triggering the animation
void idleGovernor()
{
    if (random(100) > 90)
    {
        trig[0] = 1;
    }
}

void readTrigger()
{
    // read the state of the pushbutton value:
    triggerValue = analogRead(TRIGGER_PIN);

    // check if triggerValue is greater than triggerThreshold and triggerFlag is false.
    if (triggerValue > triggerThreshold && triggerFlag == false)
    {
        // reset idle timer
        resetIdleTimer();

        // Set first element of the trigger array to 1. This will trigger the animation.
        trig[0] = 1;

        // Set the flag to true, so drum can only retrigger after being released.
        triggerFlag = true;

        // turn on onboard led
        digitalWrite(ONBOARD_LED_PIN, HIGH);
    }
    // check if triggerValue has dipped below triggerHysterisis and triggerFlag is true.
    else if (triggerValue < triggerHysterisis && triggerFlag == true)
    {
        triggerFlag = false;
        digitalWrite(ONBOARD_LED_PIN, LOW);
    }
}

// -- MAIN LOOP --

void setup()
{
    delay(3000);

    // tell FastLED about the LED beam configuration
    FastLED.addLeds<BEAM_LED_TYPE, BEAM_DATA_PIN, BEAM_COLOR_ORDER>(beamLedsRGB, BEAM_NUM_LEDS).setCorrection(TypicalLEDStrip);
    // set master BEAM_BRIGHTNESS control
    FastLED.setBrightness(BEAM_BRIGHTNESS);

    // tell FastLED about the LED DRUM configuration
    FastLED.addLeds<DRUM_LED_TYPE, DRUM_DATA_PIN, DRUM_COLOR_ORDER>(drumLedsRGB, DRUM_NUM_LEDS).setCorrection(TypicalLEDStrip);
    // set master DRUM_BRIGHTNESS control
    FastLED.setBrightness(DRUM_BRIGHTNESS);

    // initialize the onboard LED pin as an output:
    pinMode(ONBOARD_LED_PIN, OUTPUT);

    // initialize the buttonpin as an input:
    pinMode(TRIGGER_PIN, INPUT);

    // initialize serial communication at 9600 bits per second:
    Serial.begin(9600);

    // set idleTimer to current time
    idleTimer = millis();
}

void loop()
{
    readTrigger();

    switch (status)
    {
    case ACTIVE:
        updateBeamAnimation();
        updateDrumAnimation();
        checkForIdle();
        break;
    case IDLE:
        idleGovernor();
        drumIdleAnimation();
        break;
    }

    hsv2rgb();

    FastLED.show();
    FastLED.delay(1000 / FRAMES_PER_SECOND);
}