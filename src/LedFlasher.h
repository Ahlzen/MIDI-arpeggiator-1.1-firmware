#pragma once

#include <stdint.h>
#include <Arduino.h>

// Simple class for flashing an LED for
// a specified duration

class LedFlasher
{
private:
    int _pin;
    bool _ledStatus; // on/off
    unsigned long _offTime;
    unsigned int _defaultDurationMs;

public:
    LedFlasher(int pin, unsigned int defaultDurationMs);

    // call frequently (in inner loop)
    void run(unsigned long currentTime);

    // call to turn LED on
    void flash(unsigned long currentTime);
    void flash(unsigned long currentTime, unsigned int durationMs);
};