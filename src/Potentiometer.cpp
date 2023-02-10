#include <Arduino.h>
#include "Potentiometer.h"

//#define EXTRADEBUG

Potentiometer::Potentiometer(
    int adcChannel,
    unsigned int inputMin,
    unsigned int inputMax,
    unsigned int outputMin,
    unsigned int outputMax)
{
    _adcChannel = adcChannel;

    _inputMin = inputMin;
    _inputMax = inputMax;
    _outputMin = outputMin;
    _outputMax = outputMax;
    _inputRange = inputMax - inputMin;
    _outputRange = outputMax - outputMin;

    _direction = DIR_DOWN;
    _rawValue = 0;
    _sampleCount = 0;
    _sum = 0;
    _outputValue = 0;
    _hasNewValue = false;
}

void Potentiometer::sample() {
    _sum += analogRead(_adcChannel);
    _sampleCount++;
    if (_sampleCount == ANALOG_SAMPLE_COUNT) {
        // Got a full set of samples
        unsigned int newRawValue = _sum >> ANALOG_SAMPLE_SHIFT;
        _sum = 0;
        _sampleCount = 0;

        // Apply hysteresis
        if ((newRawValue > _rawValue && (_direction == DIR_UP || newRawValue > _rawValue+HYSTERESIS)) ||
            (newRawValue < _rawValue && (_direction == DIR_DOWN || newRawValue < _rawValue-HYSTERESIS)))
        {
            _direction = newRawValue > _rawValue ? DIR_UP : DIR_DOWN;
            _rawValue = newRawValue;
            // Clamp
            unsigned int value = _rawValue;
            if (value < _inputMin) value = _inputMin;
            if (value > _inputMax) value = _inputMax;
            // Calculate output value (translate input->output range)
            // NOTE: This could overflow if the ADC bits +
            // output range bits is more than the int size
            // (which would not be typical)
            value = _outputMin + (value-_inputMin)*_outputRange/_inputRange;
            if (value != _outputValue)
            {
                _hasNewValue = true;
#ifdef EXTRADEBUG
                Serial.print("Tempo: ");
                Serial.print(value);
                Serial.print("  Raw: ");
                Serial.print(_rawValue);
                Serial.print("  Direction: ");
                Serial.print(_direction == DIR_UP ? "up" : "down");
                Serial.print('\n');
#endif
            }
            _outputValue = value;
        }
    }
}

bool Potentiometer::hasNewOutputValue() {
    return _hasNewValue;
}

unsigned int Potentiometer::getRawValue() {
    return _rawValue;
}

unsigned int Potentiometer::readOutputValue() {
    _hasNewValue = false;
    return _outputValue;
}