#include <Arduino.h>
#include "Adafruit_NeoPixel.h"
#include "FastLED.h"
#include "Easing.h"

// INPUT PINS
// Define input pins
#define PIEZO_PIN_1 14
// TODO: add more pins
// Create array for all piezo pins
int piezos[]{PIEZO_PIN_1};

// OUTPUT PINS
// LED ring
#define RING_PIN 11
#define NUM_LEDS_RING 60
Adafruit_NeoPixel ring(NUM_LEDS_RING, RING_PIN, NEO_GRB + NEO_KHZ800); // LED ring // change to NUM_LEDS?

// LED beam
#define DATA_PIN 10
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS 120

// LED options
#define BRIGHTNESS_BEAM 40
#define BRIGHTNESS_RING 50
#define FRAMES_PER_SECOND 40

// Define arrays that hold led strip colour.
CRGB ledsRGB[NUM_LEDS];
CHSV ledsHSV[NUM_LEDS];
int trig[NUM_LEDS + 1];

// Define easing variables
EasingFunc<Ease::QuadInOut> e;
float start;

// Define animation variables
uint8_t currentAnimation = 0; // Index number of which pattern is current
uint8_t masterHue = 0;        // rotating "base color" used by many of the patterns

// Idle timer
unsigned long idleTimer = 0;
unsigned long idleDelay = 20000; // 5 minutes

// Piezo timer
unsigned long hitTimer = 0;
unsigned long hitDelay = 100;

// LED ring timer
unsigned long ledTimer = 0;
int ledPos = 0;
int startPos = 0;
int endPos = 0;
boolean ringIsOn = false;
boolean isIdle = false;

// Colors
int red = 255;
int green = 255;
int blue = 255;

int rounds = 0;

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
        ledsHSV[i].v = max(int(ledsHSV[i].v - random(2) * 20), 0);
        ledsHSV[i].s = min(ledsHSV[i].s + 50, 255);
    }
}

void updateAnimationFrame()
{
    // TODO: add more animations
    animationRainbowComets();
}

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
        red = random(0, 255);
        green = random(0, 255);
        blue = random(0, 255);
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
    if (hitTimer > millis() - hitDelay)
    {
        return;
    }

    hitTimer = millis(); // time is rewritten to current millis starting a new interval

    int stoot_threshold = 100;
    int shutoff_delay = 150;

    // For every sensor
    for (int i = 0; i < sizeof(piezos); i++)
    {
        // Read input
        int sensorReading = analogRead(piezos[i]);
        // if the sensor reading is greater than the threshold:
        if (sensorReading >= stoot_threshold)
        {
            Serial.println(sensorReading);
            // Serial.printf("Piezo: %d, reading: %d\n", i, sensorReading);

            // Reset idle timer, indicating action
            idleTimer = millis();

            // Play ring effect
            startPos = NUM_LEDS_RING * i;
            endPos = NUM_LEDS_RING * (i + 1);
            stootRingAnimation(startPos, endPos, random(100, 101), random(100, 101), random(100, 101));
            ringIsOn = true;

            // Change led strip effect
            if (isIdle == true)
            {
                // Reset switch
                isIdle = false;

                // Change animations
            }

            // Play led strip effect
            trig[0] = 1;

            // Reset shut-off timer for led ring, starting new interval
            ledTimer = millis();
        }
    }

    // Timer for led shut-off
    if (ringIsOn == true && ledTimer + shutoff_delay < millis())
    {
        // Reset timer
        ledTimer = millis();
        ringIsOn = false;
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

    // tell FastLED about the LED strip configuration
    FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(ledsRGB, NUM_LEDS).setCorrection(TypicalLEDStrip);

    // set master brightness control
    FastLED.setBrightness(BRIGHTNESS_BEAM);

    delay(1000);
}

void loop()
{
    readWahedStoot();

    // If there was no action for a while
    if (idleTimer + idleDelay < millis())
    {
        // Play idle animation
        idleRingAnimation(8, 30, false, 20, red, green, blue);
        isIdle = true;
    }

    updateAnimationFrame();
    hsv2rgb();
    FastLED.show();
    FastLED.delay(1000 / FRAMES_PER_SECOND);
}