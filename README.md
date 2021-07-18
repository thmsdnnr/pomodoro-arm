# pomodoro circuit express

![pomo-express](https://user-images.githubusercontent.com/18345212/126052534-fe3ad2d3-2676-417c-accc-ee3a83131831.png)

Pomdoro Circuit Express is a pomodoro timer running on the [Circuit Express](https://learn.adafruit.com/adafruit-circuit-playground-express) by Adafruit.

It's pretty simple and uses interrupts to respond to user events in the event loop. Runs with microsecond precision -- each loop takes roughly 100us or so. This is not so important for the timer precision, due to the large length of the intervals, but it does keep the device feeling snappy to user interactions.

## features

* Runs 25 minutes / 5 minutes / 15 minutes for work / short break / long break with 4x work periods for each long break per the standard [Pomodoro Technique](https://en.wikipedia.org/wiki/Pomodoro_Technique). Intervals are configurable.
* Pauses between intervals and plays a beep (C, E, G) at the end of the work, short break, and long break periods, respectively.
* Tapping the device pauses.
* When in pause mode, visually cycles between remaining NeoPixels in the current period
* Pressing left button displays number of currently completed 25 minute work periods, in base-2 (so you can visualize [0, 2^10 - 1]) periods
* Pressing right button toggles audio on/off at the end of an interval
* Switch switches the timer on and off

## future features?

* Data collection / logging with timestamp over serial
* More celebratory NeoPixel visualizations upon task completion
* More interesting tones than single notes at the end of time intervals
* Explore driving a NeoPixel string for even more time visualization fun.
