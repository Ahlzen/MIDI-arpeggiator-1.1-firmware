#pragma once
#include <stdint.h>

// Channel Voice Status

static const uint8_t MidiStatusMask = 0xF0;

static const uint8_t MidiStatusNoteOff = 0x80; // data1 = note, data2 = velocity
static const uint8_t MidiStatusNoteOn = 0x90; // data1 = note, data2 = velocity (0 = note off)
static const uint8_t MidiStatusPolyPressure = 0xA0; // data1 = note, data2 = pressure
static const uint8_t MidiStatusControlChange = 0xB0; // data1 = cc number, data2 = value
static const uint8_t MidiStatusProgramChange = 0xC0; // data1 = program number
static const uint8_t MidiStatusChannelPressure = 0xD0; // data1 = pressure
static const uint8_t MidiStatusPitchBend = 0xE0; // data1 = lsb, data2 = msb

// Channel Mode messages (CC) (selected)

static const uint8_t MidiCCAllSoundOff = 120;
static const uint8_t MidiCCLocalControl = 122; // 0 = local off, 0x7f = local on
static const uint8_t MidiCCAllNotesOff = 123;

// SysEx

static const uint8_t MidiStartOfExclusive = 0xf0;
static const uint8_t MidiEndOfExclusive = 0xf7;

// System Common messages

static const uint8_t MidiTimeCodeQuarterFrame = 0xf1;
static const uint8_t MidiSongPositionPointer = 0xf2;
static const uint8_t MidiSongSelect = 0xf3;
static const uint8_t MidiTuneRequest = 0xf6;

// System Real Time

static const uint8_t MidiTimingClock = 0xf8; // 24 ppq
static const uint8_t MidiStart = 0xfa;
static const uint8_t MidiContinue = 0xfb;
static const uint8_t MidiStop = 0xfc;
static const uint8_t MidiActiveSensing = 0xfe;
static const uint8_t MidiSystemReset = 0xff;

// Control Change numbers (selected)

static const uint8_t MidiCCPan = 10;
static const uint8_t MidiCCRegisteredParameterNumberLSB = 100;
static const uint8_t MidiCCRegisteredParameterNumberMSB = 101;

// Registered Parameter Numbers (CC 0x64 lsb, CC 0x65 msb) (selected)

static const uint8_t MidiRPNPitchBendSensitivityLSB = 0x00;
static const uint8_t MidiRPNPitchBendSensitivityMSB = 0x00;
