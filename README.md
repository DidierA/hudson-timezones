hudson-timezones
================

Multiple timezone watchface for Pebble, based on Trammell Hudson's one

The original work for this watchface is available here: https://bitbucket.org/hudson/pebble

I have ported it to SDK 2.0, and added a 4 timezones option.

To configure timezones, change src/hudson-timezones.c :

* set gmt_offset to your local timezone
* change #define NUM_TIMEZONES , and comment/uncomment the font definition macros accordingly.
* in array timezones[NUM_TIMEZONES], replace the city names and offset by the ones you want (offset is in minutes from UTC).

Build with pebble build and install with pebble install (see https://developer.getpebble.com/2/guides/ for more info)
