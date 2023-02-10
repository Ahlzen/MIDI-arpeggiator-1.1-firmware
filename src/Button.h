#pragma once

#include <stdint.h>
#include <Arduino.h>

// Simple class for reading the state of a button,
// with built-in debouncing and button-up/down/held
// (configurable) events.

class Button
{
private:
   int _pin;

   bool _pinStatus; // on/off
   unsigned long _pulseStart; // timestamp of last rising edge
   bool _buttonDownWasTriggered;
   bool _buttonHeldWasTriggered;
   
   unsigned long _debounceMs;
   unsigned long _holdMs;

public:
   Button(int pin,
      unsigned int debounceMs, // debounce time
      unsigned int holdMs = 0 // time to trigger a buttonHeld event (0=disabled)
      );

   // Call frequently (in inner loop)
   void scan(unsigned long currentTime);

   // Event handlers

   // Triggered when the button is pressed
   void (*buttonDown)() = NULL;

   // Triggered when the button is released (always)
   void (*buttonUp)() = NULL;

   // Triggered when the button is released, except
   // if the button was pressed long enough to have been
   // considered held.
   void (*buttonUpNotHeld)() = NULL;

   // Triggered when the button is pressed long enough to
   // have been considered held.
   void (*buttonHeld)() = NULL;
};