#include <Arduino.h>
#include "Adafruit_NeoPixel.h"

// INPUT
#define WAHED_DRUM_STOOT_1 15
#define WAHED_DRUM_STOOT_2 0
#define WAHED_DRUM_STOOT_3 1
#define WAHED_DRUM_STOOT_4 2
#define WAHED_DRUM_STOOT_5 3
#define NUM_PIEZOS 5
int piezos[NUM_PIEZOS]{WAHED_DRUM_STOOT_1, WAHED_DRUM_STOOT_2, WAHED_DRUM_STOOT_3, WAHED_DRUM_STOOT_4, WAHED_DRUM_STOOT_5};

// OUTPUT
#define LED_PIN_RING 14
#define NUM_RINGS 4
#define NUM_LEDS_RING 60
#define BRIGHTNESS_RING 50
const int NUM_LEDS = NUM_RINGS * NUM_LEDS_RING;
Adafruit_NeoPixel ring(NUM_LEDS_RING, LED_PIN_RING, NEO_GRB + NEO_KHZ800); // LED ring // change to NUM_LEDS?

// Piezo
unsigned long stootTimer = 0;
unsigned long stoot_delay = 100;

// LED ring
unsigned long ledTimer = 0;
int ledPos = 0;
int red = 255;
int green = 255;
int blue = 255;
int rounds = 0;

void fadeToBlack(int ledNum, byte fadeValue)
{
    uint32_t oldColor;
    uint8_t r, g, b;

    oldColor = ring.getPixelColor(ledNum);

    r = (oldColor & 0x00ff0000UL) >> 16;
    g = (oldColor & 0x0000ff00UL) >> 8;
    b = (oldColor & 0x000000ffUL);

    r = (r <= 10) ? 0 : (int)r - (r * fadeValue / 256);
    g = (g <= 10) ? 0 : (int)g - (g * fadeValue / 256);
    b = (b <= 10) ? 0 : (int)b - (b * fadeValue / 256);

    ring.setPixelColor(ledNum, r, g, b);
}

void idleRingAnimation(byte meteorSize, byte meteorTrail, boolean randomTrail, int speedDelay, int r, int g, int b)
{
    // meteorSize in amount of LEDs
    // meteorTrail determines how fast the trail dissappears (higher is faster)
    // randomTrail enables or disables random trail sharts
    // speedDelay is the delay (sync to BPM)
    // r g b are the color values

    // Guard for timing
    if (ledTimer > millis() - speedDelay)
    {
        return;
    }

    ledTimer = millis(); // time is rewritten to current millis starting a new interval

    // Position is updated
    if (ledPos < NUM_LEDS_RING)
    {
        ledPos++;
    }
    else
    {
        ledPos = 0;
        red = 0;
        green = 0;
        blue = 0;
        rounds++;
    }

    if (rounds == 2)
    {
        blue = 255;
        rounds = 0;
    }

    // Fade LEDs
    for (int j = 0; j <= NUM_LEDS_RING; j++)
    {
        if ((!randomTrail) || (random(10) > 5))
        {
            fadeToBlack(j, meteorTrail);
        }
    }
    // Draw meteor
    for (int j = 0; j < meteorSize; j++)
    {
        if (ledPos >= j)
        {
            ring.setPixelColor(ledPos - j, r, g, b);
        }
    }
    // Show LEDs
    ring.show();
}

void stootRingAnimation(int startPos, int endPos, int r, int g, int b)
{
    // For every LED
    for (int i = startPos; i < endPos; i++)
    {
        ring.setPixelColor(i, r, g, b);
    }

    // Show LEDs
    ring.show();
}

void readWahedStoot()
{
    // Guard for timing
    if (stootTimer > millis() - stoot_delay)
    {
        return;
    }

    stootTimer = millis(); // time is rewritten to current millis starting a new interval

    int stoot_threshold = 100;
    int shutoff_delay = 150;
    int startPos = 0;
    int endPos = 0;

    // if the sensor reading is greater than the threshold:
    for (int i = 0; i < NUM_PIEZOS; i++)
    {
        int sensorReading = analogRead(piezos[i]);
        if (sensorReading >= stoot_threshold)
        {
            // Serial.printf("Piezo: %d, reading: %d\n", i, sensorReading);

            startPos = NUM_LEDS_RING * i;
            endPos = NUM_LEDS_RING * (i + 1);
            stootRingAnimation(startPos, endPos, random(0, red), random(0, green), random(0, blue));

            // Reset shut-off timer
            ledTimer = millis();
        }
    }

    // Timer for led shut-off
    if (ledTimer + shutoff_delay < millis())
    {
        // Reset timer
        ledTimer = millis();
        // Turn LEDs off
        stootRingAnimation(startPos, endPos, 0, 0, 0);
    }
}

void setup()
{
    // Initialize serial communication at 9600 bits per second:
    Serial.begin(9600);

    // Initialize LED ring
    ring.begin(); // INITIALIZE NeoPixel strip object
    ring.show();  // Turn OFF all pixels ASAP
    ring.setBrightness(BRIGHTNESS_RING);

    delay(1000);
}

void loop()
{
    readWahedStoot();
    // Idle animation
    // idleRingAnimation(8, 30, false, 20, red, green, blue);
}