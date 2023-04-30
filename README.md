# MIDI-arpeggiator-1.1-firmware
Firmware for MIDI Arpeggiator v1.1

## Hardware

For Raspberry Pi Pico / RP2040

Schematics and PCB design at
https://github.com/Ahlzen/MIDI-arpeggiator-1.1-electronics

Case and panel layout at
https://github.com/Ahlzen/MIDI-arpeggiator-1.1-case

## Features

*MIDI Arpeggiatotor v1.1* is a pretty basic, standalone arpeggiator that works with pretty much anything MIDI: Hook up a MIDI keyboard to its MIDI In, and a synthesizer to its MIDI Out and jam away. Optionally, hook up a third device (such as a drum machine) to its MIDI Sync In port for external sync. Features include:

* Sync to internal tempo or external clock via MIDI.
* Snap-to-beat with external sync.
* Up, Down, Up+Down and Random mode.
* 1 to 5 octaves range.
* Hold-mode for hands-off arpeggios.
* Variable gate length (1-100% of note length).
* Dedicated buttons, knobs and LED indicators for all features.
* Using a Raspberry Pi Pico MCU with few external components.
* Easy-to-build through hole PCB with a small BOM, 3D printable case.

## Build notes

* Use PlatformIO with "Raspberry Pi Pico" board
* Update platformio.ini with
```
  platform = https://github.com/maxgerhardt/platform-raspberrypi.git
  board_build.core = earlephilhower
  ```
* On Windows: Use Zadig to install USB drivers: RPi2 boot interface -> WinUSB
* If needed: Delete a few broken packages out of .platformio\packages (auto-reinstalls)

## TODO

Missing features / TODO
* Implement Chords Mode (only arpeggiate when 2 or more keys held)
* Implement Tempo Tap (sync button, tap: tempo, hold: sync on/off)
* Use running status
