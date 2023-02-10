#include <Arduino.h>
#include "Button.h"

Button::Button(
   int pin,
   unsigned int debounceMs, // debounce time
   unsigned int holdMs // time to trigger a buttonHeld event (0=disabled)
   )
{
   _pin = pin;
   _debounceMs = debounceMs;
   _holdMs = holdMs == 0 ? UINT32_MAX : holdMs;

   _pinStatus = false; // true = button was pressed (pin pulled low by switch)
   _pulseStart = 0;

   _buttonDownWasTriggered = false;
   _buttonHeldWasTriggered = false;
}

void Button::scan(unsigned long currentTime)
{
   // Assumes pin is pulled up (not down) when open
   bool newPinStatus = digitalRead(_pin) == LOW;

   if (_pinStatus != newPinStatus)
   {
      // New status: Edge (rising or falling)
      //Serial.println(newPinStatus ? "H" : "L");
      
      if (_pinStatus == false && newPinStatus == true)
      {
         // Rising edge
         //Serial.println("Rising edge");
         _pulseStart = currentTime;
      }
      else if (_pinStatus == true && newPinStatus == false) {
         // Falling edge
         //Serial.println("Falling edge");
         unsigned long pulseLength = currentTime-_pulseStart;
         if (pulseLength >= _debounceMs) {
            if (buttonUp != NULL) buttonUp();
            if (buttonUpNotHeld != NULL &&
                !_buttonHeldWasTriggered) buttonUpNotHeld();
         }      
         _buttonDownWasTriggered = false;
         _buttonHeldWasTriggered = false;
      }
   }
   else if (newPinStatus)
   {
      // Within pulse
      unsigned long pulseLength = currentTime-_pulseStart;
      if (!_buttonDownWasTriggered && pulseLength >= _debounceMs) {
         _buttonDownWasTriggered = true;
         if (buttonDown != NULL) buttonDown();
      }
      if (!_buttonHeldWasTriggered && pulseLength >= _holdMs) {
         _buttonHeldWasTriggered = true;
         if (buttonHeld != NULL) buttonHeld();
      }
   }
   _pinStatus = newPinStatus;
}