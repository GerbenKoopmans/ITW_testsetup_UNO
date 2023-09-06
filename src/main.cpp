#include <Arduino.h>
#include "FastLED.h"
#include "Easing.h"
#include "ADS1x15.h"
#include "Wire.h"

FASTLED_USING_NAMESPACE

// LED beam options
#define BEAM_DATA_PIN 27
#define BEAM_LED_TYPE WS2812B
#define BEAM_COLOR_ORDER GRB
#define BEAM_NUM_LEDS 180
#define BEAM_BRIGHTNESS 40

// LED drum options
#define DRUM_DATA_PIN 12
#define DRUM_LED_TYPE WS2812B
#define DRUM_COLOR_ORDER GRB
#define DRUM_NUM_LEDS 60
#define DRUM_BRIGHTNESS 40

// LED options
#define FRAMES_PER_SECOND 40

// Define pins
#define TRIGGER_PIN 13
#define ONBOARD_LED_PIN 2

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

// I2C configuration

#define SDA_1 15
#define SCL_1 16
#define SDA_2 17
#define SCL_2 18
#define I2C_FREQ 400000

TwoWire I2C_sensors;
TwoWire I2C_MCU;

// ADC configuration
#define AmountOfSensors 4
ADS1115 ADC[AmountOfSensors];
float multiplier;

//MCU configuration
#define AmountOfMcu 2

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

int brightness(int trigger) // TODO: Make sure this changes the brightness accordingly
{
    if (trigger < 300)
    {
        return 50;
    }
    else if (trigger >= 300 && trigger < 700)
    {
        return 125;
    }
    return 200;
}

void beamAnimationRainbowComets()
{
    shiftToRight(trig, BEAM_NUM_LEDS);
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
    if (trig[1] == 1)
    {
        for (int i = 0; i < DRUM_NUM_LEDS; i++)
        {
            // drumLedsHSV[i].v = brightness(triggerValue);
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
        drumLedsHSV[i].s = min(drumLedsHSV[i].s + 50, 255);
    }
}

void drumIdleAnimation()
{
    // Give led a random color
    drumLedsHSV[ledPosDrum].v = idleValue;
    drumLedsHSV[ledPosDrum].h += 40;
    drumLedsHSV[ledPosDrum].s = idleHue;

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
        idleHue = random(0, 255); // TODO: Why does this not change color?
        isFullyColored = true;
    }
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
}

ADS1115 initializeAdc(ADS1115 ADC, int I2C_adress)
{
    ADC = ADS1115(&I2C_sensors, I2C_adress);
    if(!ADC.begin())
    {
        Serial.println("ADC %d Failed to start.", I2C_adress);
    }
    return ADC;
}

float readAdc()
{
    return abs(ADS.readADC_Differential_0_1() * multiplier);
}

int changeModeOtherMcuI2c(int mode)
{
    for (uint8_t i = 1; i < AmountOfMcu; i++)
    {
        Wire.beginTransmission(i);
        Wire.write(mode);
        Wire.endTransmission();
    }
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
    // Define trigger value boundaries
    int triggerThreshold = 100;
    int triggerHysterisis = 50;

    // read the state of the pushbutton value:
    triggerValue = readAdc();

    // check if triggerValue is greater than triggerThreshold and triggerFlag is false.
    if (triggerValue > triggerThreshold && triggerFlag == false)
    {
        Serial.println(triggerValue);

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

    multiplier = 0.1875F;
    I2C_sensors.begin(SDA_1, SCL_1, I2C_FREQ);
    I2C_MCU.begin(SDA_2, SCL_2, I2C_FREQ);
    
    for (int i = 0; i < AmountOfSensors; i++)
    {
        initializeAdc(ADC[i], 0x48+i);
    }
    
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
        // idleGovernor();
        drumIdleAnimation();
        break;
    }

    hsv2rgb();

    FastLED.show();
    FastLED.delay(1000 / FRAMES_PER_SECOND);
}