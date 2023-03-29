/*
Simple MIDI Arpeggiator Engine for Arduino.
(c) Lars Ahlzen 2014-2023
lars@ahlzen.com
*/

#pragma once

#include <Arduino.h>
#include <stdint.h>
#include "common.h"


class ArpEngine
{
public:
   ArpEngine(
      HardwareSerial* midiPort,
      HardwareSerial* syncPort,
      HardwareSerial* debugPort = NULL);

public:
   // Constants
   static const int MIN_TEMPO = 30; // bmp
   static const int MAX_TEMPO = 300; // bpm
   static const int MIN_GATE = 0; // %
   static const int MAX_GATE = 100; // %

   // Arpeggiator mode
   static const int MODE_UP = 0;
   static const int MODE_DOWN = 1;
   static const int MODE_UP_DOWN = 2;
   static const int MODE_RANDOM = 3;
   static const int MODE_COUNT = 4;
   // static const int MODE_ORDER = 3; // in order as played
   // static const int MODE_RANDOM1 = 4; // random: avoid same note twice in a row
   // static const int MODE_RANDOM2 = 5; // random: pick new note AND octave each time
   // static const int MODE_COUNT = 6; 

   // Note length (for MIDI sync mode)
   static const int LENGTH_WHOLE = 0;
   static const int LENGTH_HALF = 1;
   static const int LENGTH_THIRD = 2; // 1/4 triplet
   static const int LENGTH_QUARTER = 3;
   static const int LENGTH_SIXTH = 4; // 1/8 triplet
   static const int LENGTH_EIGHTH = 5;
   static const int LENGTH_TWELVTH = 6; // 1/16 triplet
   static const int LENGTH_SIXTEENTH = 7;
   static const int LENGTH_TWENTYFOURTH = 8; // 1/32 triplet
   static const int LENGTH_THIRTYSECOND = 9;
   static const int LENGTH_COUNT = 10;
   
   // Velocity mode
   static const int VEL_EACH = 0; // use each note's velocity value
   static const int VEL_SAME = 1; // use same velocity value (first note's) for all
   static const int VEL_MAX = 2; // use max value of all notes in chord
   static const int VEL_DECR = 3; // decrease velocity throughout arp
   static const int VEL_COUNT = 4;

private: // Configuration
   HardwareSerial* _midiPort;
   HardwareSerial* _syncPort;
   HardwareSerial* _debugPort;
   bool _isEnabled = false;
   bool _hold = false;
   int _mode = MODE_UP; // arp mode
   int _range = 0; // range (number of *extra* octaves)
   int _velMode = VEL_EACH; // velocity mode
   uint _tempo = 100; // 30..300 (BPM)
   uint _gate = 100; // 0..100 (%)

private: // Internal arpeggiator state

   ulong _now = 0; // current timestamp (ms)

   // Direction (e.g. for up/down mode)
   static const int DIR_UP = 0;
   static const int DIR_DOWN = 1;

   // Various constants
   //static const int MAX_OCT = 5; // max range (in octaves)
   static const int MAX_NOTES = 20; // max notes in a chord
   static const int MIDI_WAITING_FOR_STATUS_OR_DATA1 = 0;
   static const int MIDI_WAITING_FOR_DATA2 = 1;
   static const int MIDI_IN_SYSEX = 2;

   // timing and sync

   bool _midiSync = false; // true: MIDI sync mode, false: internal sync (tempo)

   // For internal tempo sync
   ulong _delayMs = 150; // 1/tempo (time between beats)
   ulong _delayMsGate = 150; // gate length
   ulong _nextOnEventAt = 0;
   ulong _nextOffEventAt = 0;
   ulong _nextBeatEventAt = 0; // onBeat event timer

   // For external MIDI sync
   // 1 MIDI beat = a 16th note = 6 clock pulses
   ulong _lastPulseAt = 0; // timestamp of last MIDI clock pulse
   ulong _pulseCounter = 0; // current MIDI pulse counter
   ulong _noteIntervalPulses = 12; // delay between notes, in clock pulses
   ulong _gateLengthPulses = 12; // gate length, in clock pulses
   ulong _nextOnEventAtPulse = 0; 
   ulong _nextOffEventAtPulse = 0;
   ulong _nextBeatEventAtPulse = 0; // onBeat event timer

   int _currentNoteNumber = 0; // note currently playing
   int _currentVelocity = 0; // current note velocity (certain vel modes only)
   int _maxVelocity = 0; // max velocity played in this chord
   int _currentNoteIndex = 0; // index in list of note currently playing
   int _currentOctave = 0; // current octave (range)
   int _lastNoteIndex = 0; // used to avoid repeated notes in RAND mode
   int _lastOctave = 0;
   int _currentDirection = DIR_UP;
   int _noteCount = 0; // # notes in current arpeggio (or chord)
   int _keyCount = 0; // # keys down (same as noteCount, except in hold mode)
   byte _noteNumbers[MAX_NOTES]; // note values (ordered by time played)
   byte _noteNumbersSorted[MAX_NOTES]; // note values (sorted, low->high)
   byte _noteVelocities[MAX_NOTES]; // note velocities (same order as noteNumbers)
   byte _noteVelocitiesSorted[MAX_NOTES]; // note velocities (same order as noteNumbersSorted)

private: // MIDI input state
   byte _midiStatus = 0; // last MIDI status: 0x80 - 0xf0
   byte _midiChannel = 0; // last MIDI channel: 0x00 - 0x0f
   byte _midiData1 = 0;
   byte _midiData2 = 0;
   byte _midiState = MIDI_WAITING_FOR_STATUS_OR_DATA1;

private: // Note data list processing
   void FillGap(byte index, byte* list);
   void MakeGap(byte index, byte* list);
   byte FindInList(byte value, byte* list);
   void AddNote(byte noteNumber, byte noteVelocity);
   void RemoveNote(byte noteNumber);

private: // MIDI output
   void SendNoteOn(byte noteNumber, byte noteVelocity);
   void SendNoteOff(byte noteNumber);
   void ForwardMidiData(byte data);
   void ForwardMidiData2Byte();
   void ForwardMidiData3Byte();

private: // MIDI input
   void HandleNoteOn();
   void HandleNoteOff();

private: // Arpeggiator logic
   void HandleMidiData(byte data); // data from MIDI in port
   void HandleSyncData(byte data); // data from sync MIDI in port
   void InitArpeggio();
   void HandleArpeggiatorOffEvent();
   void HandleArpeggiatorOnEvent();

private: // For debugging
   template <class T> void Print(T t);
   template <class T> void Print(T t, int format);
   template <class T> void PrintLn(T t);
   template <class T> void PrintLn(T t, int format);
   void PrintList(byte* list, int length);
   void PrintNoteList();
   void PrintEventSchedule();

public:
   // Call frequently (e.g. in inner loop)
   void Run(ulong now);

   void SetEnabled(bool enabled);
   void SetHold(bool hold);
   void SetTempo(int tempo); // 30-300 (BPM)
   void SetMidiSync(bool midiSyncEnabled);
   void SetGate(int gateLength); // 0..100 (%)
   void SetMode(int mode);
   void SetVelocityMode(int velocityMode);
   void SetRange(int octaves); // 0..
   
   // event handlers
   void (*onMidiIn)() = NULL; // called on any MIDI in event
   void (*onMidiOut)() = NULL; // called on any MIDI out event
   void (*onBeat)() = NULL; // called once per beat, e.g. for tempo blink
};