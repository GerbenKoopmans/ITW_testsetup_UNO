#include <Arduino.h>
#include "FastLED.h"
#include "Easing.h"
#include "ADS1x15.h"
#include "Wire.h"

FASTLED_USING_NAMESPACE

#define NUM_SYS 2 // Number of systems

// LED beam constants
#define BEAM_NUM_STRIPS 4
#define STRIP_NUM_LEDS 180
#define BEAM_BRIGHTNESS 40
#define BEAM_LED_TYPE WS2812B
#define BEAM_COLOR_ORDER GRB
const int BEAM_NUM_LEDS = BEAM_NUM_STRIPS * STRIP_NUM_LEDS;

// Beam connections 1
#define BEAM_1_1 33
#define BEAM_1_2 32
#define BEAM_1_3 12
#define BEAM_1_4 13

// Beam connections 2
#define BEAM_2_1 14
#define BEAM_2_2 27
#define BEAM_2_3 26
#define BEAM_2_4 25

// LED drum options
#define DRUM_1 2
#define DRUM_2 4

#define DRUM_LED_TYPE WS2812B
#define DRUM_COLOR_ORDER GRB
#define DRUM_NUM_LEDS 60
#define DRUM_BRIGHTNESS 40

// LED options
#define FRAMES_PER_SECOND 40

// Define pins
#define ONBOARD_LED_PIN 2

// Define arrays that hold led beam colour.
CRGB beamLedsRGB[NUM_SYS][BEAM_NUM_LEDS];
CHSV beamLedsHSV[NUM_SYS][BEAM_NUM_LEDS];

// Define array that holds led drum colour.
CRGB drumLedsRGB[NUM_SYS][DRUM_NUM_LEDS];
CHSV drumLedsHSV[NUM_SYS][DRUM_NUM_LEDS];

// Define array that holds trigger values for drum hit.
int trig[NUM_SYS][BEAM_NUM_LEDS + 1];

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

#define SDA_1 22
#define SCL_1 23
#define SDA_2 17
#define SCL_2 18
#define I2C_FREQ 400000

TwoWire I2C_sensors = TwoWire(0);
TwoWire I2C_MCU = TwoWire(1);

// ADC configuration
#define AmountOfSensors 2 // amount of ADC boards that will be activated. Addresses are counted upwards from 0x48 to 0x4b so its not possible to have 0x48 and 0x4a without 0x49.
ADS1115 ADC[4] = {ADS1115(0x48, &I2C_sensors), ADS1115(0x49, &I2C_sensors), ADS1115(0x4a, &I2C_sensors), ADS1115(0x4b, &I2C_sensors)};

float multiplier;
float ADCValues[AmountOfSensors];

// MCU configuration
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
    for (int j = 0; j < NUM_SYS; j++)
    {
        for (int i = 0; i < BEAM_NUM_LEDS; i++)
        {
            beamLedsRGB[j][i] = beamLedsHSV[j][i];
        }

        for (int i = 0; i < DRUM_NUM_LEDS; i++)
        {
            drumLedsRGB[j][i] = drumLedsHSV[j][i];
        }
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

void beamAnimationRainbowComets(int system)
{
    // shift all beams in the system to the right
    for (int i = 0; i < NUM_SYS; i++)
    {
        shiftToRight(trig[system], BEAM_NUM_LEDS);
    }

    for (int i = 0; i < BEAM_NUM_LEDS; i++)
    {
        if (trig[system][i] == 1)
        {
            beamLedsHSV[system][i].v = 200;
            beamLedsHSV[system][i].h += 40;
            beamLedsHSV[system][i].s = 0;
        }
        beamLedsHSV[system][i].v = max(int(beamLedsHSV[system][i].v - random(2) * 20), 0);
        beamLedsHSV[system][i].s = min(beamLedsHSV[system][i].s + 50, 255);
    }
}

void drumAnimation(int system)
{
    // if drum hit, fire up all leds
    if (trig[system][1] == 1)
    {
        for (int i = 0; i < DRUM_NUM_LEDS; i++)
        {
            // drumLedsHSV[i].v = brightness(triggerValue);
            drumLedsHSV[system][i].v = 200;
            drumLedsHSV[system][i].h += 40;
            drumLedsHSV[system][i].s = 0;
        }
    }

    // decrease all leds brightness
    for (int i = 0; i < DRUM_NUM_LEDS; i++)
    {
        int release = 40;

        drumLedsHSV[system][i].v = max(int(drumLedsHSV[system][i].v - release), 0);
        drumLedsHSV[system][i].s = min(drumLedsHSV[system][i].s + 50, 255);
    }
}

void drumIdleAnimation()
{

    // for all systems, update the leds
    for (int system = 0; system < NUM_SYS; system++)
    {
        // Give led a random color
        drumLedsHSV[system][ledPosDrum].v = idleValue;
        drumLedsHSV[system][ledPosDrum].h += 40;
        drumLedsHSV[system][ledPosDrum].s = idleHue;

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
}

void updateBeamAnimation()
{
    // for all systems update the beam animation
    for (int i = 0; i < NUM_SYS; i++)
    {
        beamAnimationRainbowComets(i);
    }
}

void updateDrumAnimation()
{
    // for all systems update the drum animation
    for (int i = 0; i < NUM_SYS; i++)
    {
        drumAnimation(i);
    }
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

float readAdc(int ADCNumber)
{
    return abs(ADC[ADCNumber].readADC_Differential_0_1() * multiplier);
}

// int changeModeOtherMcuI2c(int mode)
// {
//     for (uint8_t i = 1; i < AmountOfMcu; i++)
//     {
//         Wire.beginTransmission(i);
//         Wire.write(mode);
//         Wire.endTransmission();
//     }
// }

// have a random chance of triggering the animation
void idleGovernor()
{
    if (random(100) > 90)
    {
        // for all systems set the first element of the trigger array to 1
        for (int i = 0; i < NUM_SYS; i++)
        {
            trig[i][0] = 1;
        }
    }
}

void readTrigger(int ADCNumber)
{
    // Define trigger value boundaries
    int triggerThreshold = 100;
    int triggerHysterisis = 50;

    // read the state of the pushbutton value:
    triggerValue = readAdc(ADCNumber);
    ADCValues[ADCNumber] = triggerValue;

    // check if triggerValue is greater than triggerThreshold and triggerFlag is false.
    if (triggerValue > triggerThreshold && triggerFlag == false)
    {
        // reset idle timer
        resetIdleTimer();

        // Set first element of the trigger array to 1 for all systems. This will trigger the animation.
        for (int i = 0; i < NUM_SYS; i++)
        {
            trig[i][0] = 1;
        }

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

    // Beam 1 strips
    FastLED.addLeds<BEAM_LED_TYPE, BEAM_1_1, BEAM_COLOR_ORDER>(beamLedsRGB[1], 0, STRIP_NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<BEAM_LED_TYPE, BEAM_1_2, BEAM_COLOR_ORDER>(beamLedsRGB[1], STRIP_NUM_LEDS, STRIP_NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<BEAM_LED_TYPE, BEAM_1_3, BEAM_COLOR_ORDER>(beamLedsRGB[1], STRIP_NUM_LEDS * 2, STRIP_NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<BEAM_LED_TYPE, BEAM_1_4, BEAM_COLOR_ORDER>(beamLedsRGB[1], STRIP_NUM_LEDS * 3, STRIP_NUM_LEDS).setCorrection(TypicalLEDStrip);

    // Beam 2 strips)
    FastLED.addLeds<BEAM_LED_TYPE, BEAM_2_1, BEAM_COLOR_ORDER>(beamLedsRGB[2], 0, STRIP_NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<BEAM_LED_TYPE, BEAM_2_2, BEAM_COLOR_ORDER>(beamLedsRGB[2], STRIP_NUM_LEDS, STRIP_NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<BEAM_LED_TYPE, BEAM_2_3, BEAM_COLOR_ORDER>(beamLedsRGB[2], STRIP_NUM_LEDS * 2, STRIP_NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<BEAM_LED_TYPE, BEAM_2_4, BEAM_COLOR_ORDER>(beamLedsRGB[2], STRIP_NUM_LEDS * 3, STRIP_NUM_LEDS).setCorrection(TypicalLEDStrip);

    // set master BEAM_BRIGHTNESS control
    FastLED.setBrightness(BEAM_BRIGHTNESS);

    // tell FastLED about the LED DRUM configuration
    FastLED.addLeds<DRUM_LED_TYPE, DRUM_1, DRUM_COLOR_ORDER>(drumLedsRGB[1], DRUM_NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<DRUM_LED_TYPE, DRUM_2, DRUM_COLOR_ORDER>(drumLedsRGB[2], DRUM_NUM_LEDS).setCorrection(TypicalLEDStrip);
    // set master DRUM_BRIGHTNESS control
    FastLED.setBrightness(DRUM_BRIGHTNESS);

    // initialize the onboard LED pin as an output:
    pinMode(ONBOARD_LED_PIN, OUTPUT);

    // initialize serial communication at 9600 bits per second:
    Serial.begin(9600);

    // set idleTimer to current time
    idleTimer = millis();

    multiplier = 0.1875F;
    I2C_sensors.begin(SDA_1, SCL_1, I2C_FREQ);
    I2C_MCU.begin(SDA_2, SCL_2, I2C_FREQ);

    for (int i = 0; i < AmountOfSensors; i++)
    {
        if (!ADC[i].begin())
        {
            Serial.printf("ADC %d connection with address %x failed.", i + 1, i + 0x48);
            while (true)
                ;
        }
    }
}

void loop()
{
    for (int i = 0; i < AmountOfSensors; i++)
    {
        readTrigger(i);
    }

    for (int i = 0; i < AmountOfSensors; i++)
    {
        if (i + 1 != AmountOfSensors)
        {
            Serial.printf("%f ", ADCValues[i]);
        }
        else
        {
            Serial.printf("%f", ADCValues[i]);
        }
    }

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