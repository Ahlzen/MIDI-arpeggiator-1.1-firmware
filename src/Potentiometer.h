#pragma once

#include <stdint.h>

// ADC potentiometer input helper with:
// * oversampling (for noise reduction)
// * hysteresis (for stability)
// * input->output range translation

class Potentiometer
{
private:
    static const unsigned int ANALOG_SAMPLE_COUNT = 64; // for averaging
    static const unsigned int ANALOG_SAMPLE_SHIFT = 6; // bits to shift right

    static const unsigned int HYSTERESIS = 5; // in sample values

    static const bool DIR_DOWN = false;
    static const bool DIR_UP = true;


    int _adcChannel;

    // For translating input=>output
    unsigned int _inputMin, _inputMax;
    unsigned int _outputMin, _outputMax;
    unsigned int _inputRange, _outputRange;

    unsigned int _rawValue;
    unsigned int _sampleCount; 
    unsigned int _sum;

    bool _direction; // for hysteresis

    unsigned int _outputValue;
    bool _hasNewValue;

public:
    Potentiometer(
        int adcChannel,
        unsigned int inputMin, unsigned int inputMax,
        unsigned int outputMin, unsigned int outputMax);

    // Call this frequently. When enough samples have been read
    // an updated value is calculated.
    void sample();

    // Returns true iff there's a new (different) value available.
    bool hasNewOutputValue();

    unsigned int getRawValue();

    // Returns the current output value and clears the hasNewValue flag.
    unsigned int readOutputValue();
};

