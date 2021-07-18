/**
 * pomodoro.cpp is a pomodoro timer designed for an
 * Adafruit Circuit Playground Express:
 * https://learn.adafruit.com/adafruit-circuit-playground-express
* */

// https://github.com/adafruit/Adafruit_CircuitPlayground
#include <Adafruit_CircuitPlayground.h>

// ATTRIBUTION: Tap code from:
// https://github.com/adafruit/Adafruit_CircuitPlayground/blob/master/examples/accelTap/accelTap.ino

// Adjust this number for the sensitivity of the 'click' force
// this strongly depend on the range! for 16G, try 5-10
// for 8G, try 10-20. for 4G try 20-40. for 2G try 40-80
#define CLICKTHRESHHOLD 40

#define PITCH_C3 130
#define PITCH_E3 164
#define PITCH_G3 196
#define SOUND_DURATION_MS 50

// Number of lights available to illuminate on the board.
// Circuit Playground Express has 10.
#define CT_NEOPIXELS 10

// 25 minutes work, 5 minutes short break, 15 minutes long break = 1500, 300, 900
#define WORK_SECONDS 1500 // 25 minutes work = 1500 seconds
#define WORK_US 1000000 * WORK_SECONDS
#define WORK_COLOR 0xff0b0b

#define SBRK_SECONDS 300 // 5 minutes break = 300 seconds
#define SBRK_US 1000000 * SBRK_SECONDS
#define SBRK_COLOR 0xff0aff

#define LBRK_SECONDS 900 // 15 minutes long break = 900 seconds
#define LBRK_US 1000000 * LBRK_SECONDS
#define LBRK_COLOR 0x0affff

// number of work sessions before long break (usually 4)
#define NUM_WORK_BEFORE_LONG_BREAK 4

// Illuminate numPixels NeoPixels in base-2 using color (works for [0, 2^10 - 1])
void drawNLightsBinaryWithColor(int numPixels, int color)
{
    CircuitPlayground.clearPixels();
    int lightNum = 0;
    while (numPixels > 0)
    {
        // Test the LSB, if lit, light the pixel.
        if (numPixels & 1)
        {
            CircuitPlayground.setPixelColor(lightNum, color);
        }
        // Throw away the LSB.
        numPixels >>= 1;
        lightNum++;
    }
}

// Illuminate numPixels NeoPixels in base-10 using color.
void drawNLightsWithColor(int numPixels, int color)
{
    CircuitPlayground.clearPixels();
    for (int i = 0; i < numPixels; i++)
        CircuitPlayground.setPixelColor(i, color);
}

// 3 states: work, short break, long break.
const int colors[3] = {WORK_COLOR, SBRK_COLOR, LBRK_COLOR};
const int sounds[3] = {PITCH_C3, PITCH_E3, PITCH_G3};
const long durations[3] = {WORK_US, SBRK_US, LBRK_US};

// Hold counter state: one of work, short break, long break
short state = 0;
int color = colors[state];
int sound = sounds[state];
long duration = durations[state];

// Light up fraction of lights based on time passed
int numPixels = 10;
int lastNumPixels = 10;

// Pomodoros in cycle before long break
int thisCyclePomoCt = 0;

// Total number of pomodoros
int totalPomoCt = 0;

// Time is tracked in ticks of microsecond precision.
unsigned long lastMicros = micros();

volatile bool isPaused = false;
volatile bool didTogglePause = false;
void togglePaused(void)
{
    isPaused = !isPaused;
    didTogglePause = true;
}

volatile bool displayStats = false;
void toggleDisplayMode(void)
{
    displayStats = !displayStats;
    // Kind of a hack to prevent tap from button press triggering.
    isPaused = false;
}

volatile bool playTones = true;
void togglePlayTones(void)
{
    playTones = !playTones;
    // Kind of a hack to prevent tap from button press triggering.
    isPaused = false;
}

volatile bool isOn = true;
void toggleIsOn(void)
{
    isOn = !isOn;
    // Kind of a hack to prevent tap from button press triggering.
    isPaused = false;
}

void loop()
{
    if (isOn)
    {
        if (displayStats)
        {
            drawNLightsBinaryWithColor(totalPomoCt, WORK_COLOR);
            delay(2000);
            displayStats = false;
        }

        if (isPaused)
        {
            // Start at the beginning
            while (isPaused)
            {
                // Turn off all the NeoPixels
                CircuitPlayground.clearPixels();
                delay(242);
                for (int pixelIdx = 0; pixelIdx < numPixels; pixelIdx++)
                {
                    CircuitPlayground.setPixelColor(pixelIdx, color);
                    delay(42);
                }
                // Wait a little bit so we don't spin too fast
                delay(1042);
            }
        }
        else
        {
            unsigned long thisMicros = micros();
            unsigned long timePassed = thisMicros - lastMicros;
            lastMicros = thisMicros;

            if (didTogglePause)
            {
                drawNLightsWithColor(numPixels, color);
                didTogglePause = false;
                // we were paused, so ignore the time passed while paused :D
                timePassed = 0;
            }

            // No state transition.
            if (duration >= 0)
            {
                // Display fractional number of lights depending on state.
                // We have to scale the duration by 2**4 here in order to prevent overflow
                // Only want to get the fraction complete so far for the given state period
                // to illuminate that integral percentage of lights.
                numPixels = min(CT_NEOPIXELS, 1 + (CT_NEOPIXELS * (duration >> 4) / (durations[state] >> 4)));
                if (numPixels != lastNumPixels)
                {
                    drawNLightsWithColor(numPixels, color);
                    lastNumPixels = numPixels;
                }
                duration -= timePassed;
            }

            // State transition.
            if (duration < 0)
            {
                if (state == 0)
                {
                    // In work state.
                    thisCyclePomoCt++;
                    totalPomoCt++;
                    if (thisCyclePomoCt == NUM_WORK_BEFORE_LONG_BREAK)
                    {
                        // Go to long break.
                        thisCyclePomoCt = 0;
                        state = 2;
                    }
                    else
                    {
                        // Go to short break.
                        state = 1;
                    }
                }
                else
                {
                    // We were in a break state -- go to work.
                    state = 0;
                }

                // Update color, sound, duration for current state.
                color = colors[state];
                sound = sounds[state];
                duration = durations[state];

                // Play an end-of-state tone.
                if (playTones)
                    CircuitPlayground.playTone(sound, SOUND_DURATION_MS);
                drawNLightsWithColor(10, color);

                // We pause at each state transition to wait for user interaction.
                isPaused = true;
            }
        }
    }
    else
    {
        // If off, turn off pixels :)
        CircuitPlayground.clearPixels();
    }
}

void setup(void)
{
    CircuitPlayground.begin();
    // Attach interrupts to sense taps
    // 0 = turn off click detection & interrupt
    // 1 = single click only interrupt output
    // 2 = double click only interrupt output, detect single click
    // Adjust threshhold, higher numbers are less sensitive
    CircuitPlayground.setAccelRange(LIS3DH_RANGE_2_G); // 2, 4, 8 or 16 G!
    CircuitPlayground.setAccelTap(1, CLICKTHRESHHOLD);
    attachInterrupt(digitalPinToInterrupt(CPLAY_LIS3DH_INTERRUPT), togglePaused, FALLING);

    // Left button pressed, shows stats.
    attachInterrupt(digitalPinToInterrupt(4), toggleDisplayMode, FALLING);

    // Right button pressed, toggles playing end-of-cycle tones.
    attachInterrupt(digitalPinToInterrupt(5), togglePlayTones, FALLING);

    // On-off switch.
    attachInterrupt(digitalPinToInterrupt(7), toggleIsOn, CHANGE);

    // Set NeoPixels to not be super-bright.
    CircuitPlayground.setBrightness(10);

    drawNLightsWithColor(10, colors[0]);
}
