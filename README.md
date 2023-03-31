# MIDI-arpeggiator-1.1-firmware
Firmware for MIDI Arpeggiator v1.1

## Hardware

For Raspberry Pi Pico / RP2040

TODO: More details

## Build notes

* Use PlatformIO with "Raspberry Pi Pico" board
* Update platformio.ini with
  platform = https://github.com/maxgerhardt/platform-raspberrypi.git
  board_build.core = earlephilhower
* On Windows: Use Zadig to install USB drivers: RPi2 boot interface -> WinUSB

If needed: Delete a few broken packages out of .platformio\packages (auto-reinstalls)

## TODO

Bugs

* Weird stuck notes in other modes [Done]
* Gate not working quite right [Done]
* Fix double first-note in up/down mode [Done]

Missing features

* Tempo LED [Done]
* Enable hold without turning arp on/off (fix button events) [Done]
* Remaining LEDs / buttons [Done]
* Implement MIDI Sync [Done]
* Implement Chords Mode (only arpeggiate when 2 or more keys held)
* Implement Tempo Tap (sync button, tap: tempo, hold: sync on/off)

