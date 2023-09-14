#include <Arduino.h>
#include "FastLED.h"
#include "ADS1x15.h"

FASTLED_USING_NAMESPACE

#define NUM_SYS 2

#define BEAM_BRIGHTNESS 255
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define BEAM_NUM_LEDS 720

#define BEAM_1_1 33
#define BEAM_1_2 32
#define BEAM_1_3 12
#define BEAM_1_4 13

#define BEAM_2_1 14
#define BEAM_2_2 27
#define BEAM_2_3 26
#define BEAM_2_4 25

#define DRUM_1 2
#define DRUM_2 4
#define DRUM_NUM_LEDS 60
#define DRUM_BRIGHTNESS 255

#define FRAMES_PER_SECOND 1000

CRGB beamLedsRGB[NUM_SYS][BEAM_NUM_LEDS];
CHSV beamLedsHSV[NUM_SYS][BEAM_NUM_LEDS];
CRGB drumLedsRGB[NUM_SYS][DRUM_NUM_LEDS];
CHSV drumLedsHSV[NUM_SYS][DRUM_NUM_LEDS];

int trig[NUM_SYS][BEAM_NUM_LEDS + 1];

int triggerValue = 0;
bool triggerFlag = false;

unsigned long idleTimer = 0;
unsigned long idleThreshold = 300000;

int ledPosDrum = 0;
int idleHue = 100;
int idleValue = 200;
bool isFullyColored = true;

#define SDA_1 22
#define SCL_1 23
#define SDA_2 17
#define SCL_2 18
#define I2C_FREQ 400000

TwoWire I2C_sensors = TwoWire(0);
TwoWire I2C_MCU = TwoWire(1);

#define AmountOfSensors 2
ADS1115 ADC[AmountOfSensors] = {ADS1115(0x48, &I2C_sensors), ADS1115(0x49, &I2C_sensors)};

float multiplier;
float ADCValues[AmountOfSensors];

uint8_t status;
enum status_types_t
{
    ACTIVE,
    IDLE
};

void readTrigger(int ADCNumber)
{
    int triggerThreshold = 5;
    int triggerHysterisis = 1;

    triggerValue = abs(ADC[ADCNumber].readADC_Differential_0_1() * multiplier);
    ADCValues[ADCNumber] = triggerValue;

    if (triggerValue > triggerThreshold && !triggerFlag)
    {
        resetIdleTimer();
        trig[ADCNumber][0] = 1;
        Serial.println(triggerValue);
        triggerFlag = true;
    }
    else if (triggerValue < triggerHysterisis && triggerFlag)
    {
        triggerFlag = false;
    }
}

void updateBeamAnimation()
{
    for (int i = 0; i < NUM_SYS; i++)
    {
        shiftToRight(trig[i]);
        for (int j = 0; j < BEAM_NUM_LEDS; j++)
        {
            if (trig[i][j] == 1)
            {
                beamLedsHSV[i][j].v = 200;
                beamLedsHSV[i][j].h += 40;
                beamLedsHSV[i][j].s = 0;
            }
            else
            {
                beamLedsHSV[i][j].v = max(int(beamLedsHSV[i][j].v - random(2) * 20), 0);
                beamLedsHSV[i][j].s = min(beamLedsHSV[i][j].s + 50, 255);
            }
        }
    }
}

void updateDrumAnimation()
{
    for (int i = 0; i < NUM_SYS; i++)
    {
        for (int j = 0; j < DRUM_NUM_LEDS; j++)
        {
            if (trig[i][1] == 1)
            {
                drumLedsHSV[i][j].v = 200;
                drumLedsHSV[i][j].h += 40;
                drumLedsHSV[i][j].s = 0;
            }
            else
            {
                int release = 40;
                drumLedsHSV[i][j].v = max(int(drumLedsHSV[i][j].v - release), 0);
                drumLedsHSV[i][j].s = min(drumLedsHSV[i][j].s + 50, 255);
            }
        }
    }
}

void drumIdleAnimation()
{
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
        else if (isFullyColored)
        {
            ledPosDrum = 0;
            idleValue = 0;
            isFullyColored = false;
        }
        else if (!isFullyColored)
        {
            ledPosDrum = 0;
            idleValue = 200;
            idleHue = random(0, 255);
            isFullyColored = true;
        }
    }
}

void checkForIdle()
{
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

void hsv2rgb()
{
    for (int i = 0; i < NUM_SYS; i++)
    {
        for (int j = 0; j < BEAM_NUM_LEDS; j++)
        {
            beamLedsRGB[i][j] = beamLedsHSV[i][j];
        }
        for (int j = 0; j < DRUM_NUM_LEDS; j++)
        {
            drumLedsRGB[i][j] = drumLedsHSV[i][j];
        }
    }
}

void shiftToRight(int a[])
{
    memmove(a + 1, a, sizeof(a) - sizeof(a[0]));
    a[0] = 0;
}

void setup()
{
    delay(3000);

    FastLED.addLeds<LED_TYPE, BEAM_1_1, COLOR_ORDER>(beamLedsRGB[0], 0, 180).setCorrection(TypicalLEDStrip);
    // Add configurations for other beam LEDs here.

    FastLED.setBrightness(BEAM_BRIGHTNESS);

    FastLED.addLeds<LED_TYPE, DRUM_1, COLOR_ORDER>(drumLedsRGB[0], DRUM_NUM_LEDS).setCorrection(TypicalLEDStrip);
    // Add configurations for other drum LEDs here.

    FastLED.setBrightness(DRUM_BRIGHTNESS);

    Serial.begin(9600);
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
        // Read trigger values and perform actions.
        readTrigger(i);
    }

    switch (status)
    {
    case ACTIVE:
        updateBeamAnimation();
        updateDrumAnimation();
        checkForIdle();
        break;
    case IDLE:
        drumIdleAnimation();
        break;
    }

    hsv2rgb();

    FastLED.show();
}