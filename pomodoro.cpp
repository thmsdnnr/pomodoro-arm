/**
 * pomodoro.cpp is a pomodoro timer designed for an
 * Adafruit Circuit Playground Express:
 * https://learn.adafruit.com/adafruit-circuit-playground-express
 * https://github.com/adafruit/Adafruit_CircuitPlayground
* */

#include <Adafruit_CircuitPlayground.h>

// Frequencies and sound durations for end-of-cycle tones.
#define PITCH_C3 130
#define PITCH_E3 164
#define PITCH_G3 196
#define SOUND_DURATION_MS 50

// How hard to tap for detection. Lower number = less force.
#define TAP_THRESHOLD_FORCE 15

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

// Interrupt service routines to react to user HW interactions
volatile bool isPaused = false;
volatile bool didTogglePause = false;
void togglePaused(void)
{
    isPaused = !isPaused;
    didTogglePause = true;
}

volatile bool playTones = true;
void togglePlayTones(void)
{
    playTones = !playTones;
}

volatile bool isOn = true;
void toggleIsOn(void)
{
    isOn = !isOn;
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

// Main app loop.
void loop()
{
    if (!isOn)
    {
        // If off, turn off pixels :)
        CircuitPlayground.clearPixels();
    }
    else
    {
        while (isPaused)
        {
            // Draw number of completed pomodors in binary,
            // then stream through all neopixels with current
            // state color until user taps to resume.
            CircuitPlayground.clearPixels();
            drawNLightsBinaryWithColor(totalPomoCt, WORK_COLOR);
            delay(424);
            for (int pixelIdx = 0; pixelIdx < numPixels; pixelIdx++)
            {
                CircuitPlayground.setPixelColor(pixelIdx, color);
                delay(42);
            }
            delay(242);
        }

        // Compute time elapsed since last tick, and if unpaused,
        // subtract from total time remaining for current state.
        unsigned long thisMicros = micros();
        unsigned long timePassed = thisMicros - lastMicros;
        lastMicros = thisMicros;

        // Each loop computes a timePassed, but we want to ignore
        // it if the device is paused.
        if (didTogglePause)
        {
            drawNLightsWithColor(numPixels, color);
            didTogglePause = false;
            // Ignore the time passed while paused.
            timePassed = 0;
        }

        // No state transition.
        if (duration >= 0)
        {
            // Display num lights * (percent completed) for current state.
            // Scale the duration by 2**4 in order to prevent overflow.
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
                    // Work -> Long Break
                    thisCyclePomoCt = 0;
                    state = 2;
                }
                else
                {
                    // Work -> Short Break
                    state = 1;
                }
            }
            else
            {
                // {Long Break || Short Break} -> Work
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
            numPixels = CT_NEOPIXELS;
        }
    }
}

// Initialize the hardware and attach interrupt handlers.
void setup(void)
{
    CircuitPlayground.begin();

    // I want the switch to be on if it's flipped right :)
    // but slideSwitch returns True if it's flipped left.
    isOn = !CircuitPlayground.slideSwitch();

    // A tap toggles pause. ATTRIBUTION: Tap code from:
    // https://github.com/adafruit/Adafruit_CircuitPlayground/blob/master/examples/accelTap/accelTap.ino

    CircuitPlayground.setAccelRange(LIS3DH_RANGE_2_G);
    CircuitPlayground.setAccelTap(1, TAP_THRESHOLD_FORCE);
    attachInterrupt(digitalPinToInterrupt(CPLAY_LIS3DH_INTERRUPT), togglePaused, FALLING);

    // Right button pressed, toggles playing end-of-cycle tones.
    attachInterrupt(digitalPinToInterrupt(5), togglePlayTones, FALLING);

    // On-off switch.
    attachInterrupt(digitalPinToInterrupt(7), toggleIsOn, CHANGE);

    // Set NeoPixels to not be super-bright.
    CircuitPlayground.setBrightness(10);

    drawNLightsWithColor(10, colors[0]);
}
