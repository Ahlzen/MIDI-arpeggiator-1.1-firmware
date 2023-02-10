#include <Arduino.h>
#include "LedFlasher.h"

LedFlasher::LedFlasher(int pin, unsigned int defaultDurationMs)
{
    _pin = pin;
    _defaultDurationMs = defaultDurationMs;
    _ledStatus = false;
    _offTime = 0;
}

void LedFlasher::run(unsigned long currentTime)
{
    if (_ledStatus && currentTime >= _offTime) {
        digitalWrite(_pin, LOW);
        _ledStatus = false;
    }
}

void LedFlasher::flash(unsigned long currentTime)
{
    this->flash(currentTime, _defaultDurationMs);
}

void LedFlasher::flash(unsigned long currentTime, unsigned int durationMs)
{
    if (!_ledStatus) {
        digitalWrite(_pin, HIGH);
        _ledStatus = true;
    }
    if (_offTime < (currentTime+durationMs)) {
        _offTime = currentTime+durationMs;
    }
}