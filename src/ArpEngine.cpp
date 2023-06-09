#include "common.h"
#include "MIDI.h"
#include "ArpEngine.h"


ArpEngine::ArpEngine(
  HardwareSerial* midiPort,
  HardwareSerial* syncPort,
  HardwareSerial* debugPort)
{
  _midiPort = midiPort;
  _syncPort = syncPort;
  _debugPort = debugPort;
  _now = millis(); // initial value, then supplied through run()
}


///////// NOTE DATA LIST PROCESSING

void ArpEngine::FillGap(byte index, byte* list) {
  for (byte p = index; p < _noteCount-1; p++) {
    list[p] = list[p+1];
  }
}

void ArpEngine::MakeGap(byte index, byte* list) {
  for (byte p = _noteCount; p > index; p--) {
    list[p] = list[p-1];
  }
}

// returns MAX_NOTES if not found
byte ArpEngine::FindInList(byte value, byte* list) {
  byte i = 0;
  while (i < MAX_NOTES && list[i] != value) i++;
  return i;
}

// Adds the specified note+vel to the list of notes
// currently playing and updates noteCount
void ArpEngine::AddNote(byte noteNumber, byte noteVelocity)
{
  Print("AddNote: "); Print(noteNumber); Print(", "); PrintLn(noteVelocity);

  if (_noteCount == MAX_NOTES) return; // list is full
    
  // Insert in sorted list (also checks that it
  // doesn't already exist; possible if hold enabled)
  byte index = 0;
  while (index < _noteCount && _noteNumbersSorted[index] < noteNumber) index++;
  if (index < _noteCount && _noteNumbersSorted[index] == noteNumber)
  {
    PrintLn("  already in list:");
    PrintNoteList();
    return; // already in list
  }
   
  // Insert in sorted list
  MakeGap(index, _noteNumbersSorted);
  MakeGap(index, _noteVelocitiesSorted);
  _noteNumbersSorted[index] = noteNumber;
  _noteVelocitiesSorted[index] = noteVelocity;
  
  // Insert in ordered list
  _noteNumbers[_noteCount] = noteNumber;
  _noteVelocities[_noteCount] = noteVelocity;

  _noteCount++;
  PrintNoteList();
}

// Removes the specified note from the list of notes
// currently playing and updates noteCount
void ArpEngine::RemoveNote(byte noteNumber)
{
  Print("RemoveNote: "); PrintLn(noteNumber);
  if (_noteCount == 0) return; // nothing to remove
 
  // Remove from ordered list
  byte index = FindInList(noteNumber, _noteNumbers);
  if (index < MAX_NOTES) {
    FillGap(index, _noteNumbers);
    FillGap(index, _noteVelocities);
  }
  
  // Remove from sorted list
  index = FindInList(noteNumber, _noteNumbersSorted);
  if (index < MAX_NOTES) {
    FillGap(index, _noteNumbersSorted);
    FillGap(index, _noteVelocitiesSorted);
  }  
  
  _noteCount--;
  PrintNoteList();
}



///////// MIDI OUTPUT

// TODO: Use running status when sending

void ArpEngine::SendNoteOn(byte noteNumber, byte noteVelocity) {
  _midiPort->write(0x90 + _midiChannel);
  _midiPort->write(noteNumber);
  _midiPort->write(noteVelocity);
  //Print("SendNoteOn: "); PrintLn(noteNumber);
  if (onMidiOut != NULL) onMidiOut();
}

void ArpEngine::SendNoteOff(byte noteNumber) {
  _midiPort->write(0x90 + _midiChannel);
  _midiPort->write(noteNumber);
  _midiPort->write((uint8_t)0); // velocity 0 = note off
  //Print("SendNoteOff: "); PrintLn(noteNumber);
  if (onMidiOut != NULL) onMidiOut();
}

void ArpEngine::ForwardMidiData(byte data) {
  _midiPort->write(data);
  if (onMidiOut != NULL) onMidiOut();
}

void ArpEngine::ForwardMidiData2Byte() {
  _midiPort->write(_midiStatus + _midiChannel);
  _midiPort->write(_midiData1);
  if (onMidiOut != NULL) onMidiOut();
}

void ArpEngine::ForwardMidiData3Byte() {
  _midiPort->write(_midiStatus + _midiChannel);
  _midiPort->write(_midiData1);
  _midiPort->write(_midiData2);
  if (onMidiOut != NULL) onMidiOut();
}



///////// MIDI INPUT

void ArpEngine::HandleNoteOn()
{
  if (_midiData2 == 0) // actually note off
  {
    HandleNoteOff();
    return;
  }
  
  _keyCount++;

  // special case: first key down cancels a
  // held arpeggio
  if (_hold && _keyCount == 1) {
    SendNoteOff(_currentNoteNumber);
    _noteCount = 0;
  }

  int noteNumber = _midiData1;
  int noteVelocity = _midiData2;
  AddNote(noteNumber, noteVelocity);

  if (_isEnabled)
  {
    if (_noteCount == 1) // first note
    {
      InitArpeggio();
    }
  }
  else
  {
    ForwardMidiData3Byte(); // pass note-on through
  }
  _maxVelocity = max(noteVelocity, _maxVelocity);  
}

void ArpEngine::HandleNoteOff()
{
  int noteNumber = _midiData1;
  
  if (!_isEnabled || (_isEnabled && !_hold))
  {
    RemoveNote(noteNumber);
  }
  _keyCount--;

  if (_isEnabled)
  {
    if (_keyCount == 0 && !_hold) // last note - stop arpeggiator (unless hold)
    {
      PrintLn("Arpeggio ended.");
      SendNoteOff(_currentNoteNumber);
    }
  }
  else
  {
    ForwardMidiData3Byte(); // pass note-off through
  }
}



///////// ARPEGGIATOR LOGIC

void ArpEngine::HandleMidiData(byte data)
{
  if (_midiState == MIDI_IN_SYSEX) {
    ForwardMidiData(data);
    if (data == MidiEndOfExclusive) {
      // end of SysEx
      _midiState = MIDI_WAITING_FOR_STATUS_OR_DATA1;
      _midiStatus = 0;
    }
    return;
  }
  
  if (data & MidiStatusByteMask) {
    // status byte
    if (data == MidiStartOfExclusive) {
      // start of SysEx
      ForwardMidiData(data);
      _midiState = MIDI_IN_SYSEX;
    }
    else if ((data & MidiStatusMask) == MidiStatusSystemMessage) {
      // Other system message: pass through
      // (these are only relevant on the sync port)
      ForwardMidiData(data);
      _midiStatus = 0;
    } else {
      // channel message
      _midiStatus = data & MidiStatusMask;
      _midiChannel = data & MidiChannelMask;
      _midiState = MIDI_WAITING_FOR_STATUS_OR_DATA1;
    }
  } else {
    // data byte
    switch (_midiState) {
      case MIDI_WAITING_FOR_STATUS_OR_DATA1: // first data byte
        _midiData1 = data;
        switch (_midiStatus) {
          case MidiStatusNoteOff:
          case MidiStatusNoteOn:
          case MidiStatusPolyPressure:
          case MidiStatusControlChange:
          case MidiStatusPitchBend:
            _midiState = MIDI_WAITING_FOR_DATA2;
            break;
          case MidiStatusProgramChange:
          case MidiStatusChannelPressure:
            ForwardMidiData2Byte(); // pass-through
            break;
        }
        break;
      case MIDI_WAITING_FOR_DATA2: // second data byte
        _midiData2 = data;
        switch (_midiStatus) {
          case MidiStatusNoteOff:
            HandleNoteOff();
            break;
          case MidiStatusNoteOn:
            HandleNoteOn();
            break;
          default: // other
            ForwardMidiData3Byte(); // pass-through
            break;
        }
        _midiState = MIDI_WAITING_FOR_STATUS_OR_DATA1;
        break;
    }
  }
}

void ArpEngine::HandleSyncData(byte data)
{
  if (data == MidiTimingClock) {
    _pulseCounter++;
    _lastPulseAt = _now;
  }
  else if (data == MidiStart) {
    // Reset pulse counter etc so snap-to-beat/quantization works
    _pulseCounter = 0;
    _nextOnEventAtPulse = 0;
    _nextOffEventAtPulse = 0;
    _nextBeatEventAtPulse = 0;
  }
}

void ArpEngine::InitArpeggio()
{
  PrintLn("InitArpeggio()");
  SendNoteOn(_noteNumbers[0], _noteVelocities[0]);
  _currentNoteNumber = _noteNumbers[0];
  _currentNoteIndex = 0;
  _currentOctave = 0;
  _currentVelocity = _noteVelocities[0];
  switch (_mode) {
    case MODE_DOWN:
      _currentDirection = DIR_DOWN; break;
    default:
      _currentDirection = DIR_UP; break;
  }

  if (_midiSync) {
    _nextOnEventAtPulse = _pulseCounter + _noteIntervalPulses;
    _nextOffEventAtPulse = _pulseCounter + _gateLengthPulses;
  }
  else {
    _nextOnEventAt = _now + _delayMs;
    _nextOffEventAt = _now + _delayMsGate;
  }
  
  PrintEventSchedule();
  _maxVelocity = _noteVelocities[0];
}

void ArpEngine::HandleArpeggiatorOffEvent()
{
  SendNoteOff(_currentNoteNumber);
  _lastOctave = _currentOctave;
  _lastNoteIndex = _currentNoteIndex; 
}

void ArpEngine::HandleArpeggiatorOnEvent()
{
  bool restartVelocity = false;
  
  // Advance one step (this is mode dependent)
  switch (_mode) {
    case MODE_UP:
    //case MODE_ORDER:
    {
      _currentNoteIndex++;
      if (_currentNoteIndex >= _noteCount) {
        _currentNoteIndex = 0;
        _currentOctave++;
        if (_currentOctave > _range) {
          _currentOctave = 0;
          restartVelocity = true;
        }
      }
      break;
    }
    case MODE_DOWN:
    {
      _currentNoteIndex--;
      if (_currentNoteIndex < 0) {
         _currentNoteIndex = _noteCount-1;
         _currentOctave--;
         if (_currentOctave < 0) {
            _currentOctave = _range;
            restartVelocity = true;
         }
      }
      break;
    }
    case MODE_UP_DOWN:
    {
      if (_noteCount < 2 && _range == 0) break; // single note
      if (_currentDirection == DIR_UP) {
         _currentNoteIndex++;
         if (_currentNoteIndex >= (_noteCount-1) && _currentOctave == _range) {
           // reached the top. turn around!
           _currentNoteIndex = _noteCount-1;
           _currentDirection = DIR_DOWN;      
         } else if (_currentNoteIndex >= _noteCount) {
           // run next octave up
           _currentNoteIndex = 0;
           _currentOctave++;
         } 
      }
      else // currentDirection == DIR_DOWN
      {
         _currentNoteIndex--;
         if (_currentNoteIndex <= 0 && _currentOctave == 0) {
           // reached the bottom. turn around!
           _currentNoteIndex = 0;
           _currentDirection = DIR_UP;
           restartVelocity = true;
         } else if (_currentNoteIndex < 0) {
           // run next octave down
           _currentNoteIndex = _noteCount-1;
           _currentOctave--;
         }
      }
      break;
    }
    //case MODE_RANDOM1:
    case MODE_RANDOM:
    {
      if (_noteCount < 2 && _range == 0) break; // single note
      
      // Avoid playing the same note twice in a row:
      while (_currentNoteIndex == _lastNoteIndex && _currentOctave == _lastOctave)
      {
         // TODO: Replace rand by 8-bit LFSR?
         _currentNoteIndex = random(_noteCount);
         _currentOctave = random(_range+1);
      }
      break;
    }
    // TODO: Enable
    // case MODE_RANDOM2:
    // {
    //   if (_noteCount < 2 && _range == 0) break; // single note
      
    //   // Try to pick both a different note and octave than last time
    //   if (_noteCount > 1) while (_currentNoteIndex == _lastNoteIndex)
    //     _currentNoteIndex = random(_noteCount);
    //   if (_range > 0) while (_currentOctave == _lastOctave)
    //     _currentOctave = random(_range+1);
    //   break;
    // }
    default: break;
  }
  
  // Find next note
  
  byte* noteNumberList = _noteNumbersSorted;
  byte* noteVelocityList = _noteVelocitiesSorted;
  // if (_mode == MODE_ORDER) {
  //   // use ordered list instead of sorted
  //   noteNumberList = _noteNumbers;
  //   noteVelocityList = _noteVelocities;
  // }
  byte noteNumber = noteNumberList[_currentNoteIndex] + 12 * _currentOctave;
  
  // Find what velocity value to use

  //byte noteVelocity;
  switch (_velMode)
  {
    case VEL_EACH:
      _currentVelocity = noteVelocityList[_currentNoteIndex];
      break;
    case VEL_SAME:
      _currentVelocity = noteVelocityList[0];
      break;
    case VEL_MAX:
      _currentVelocity = _maxVelocity;
      break;
    case VEL_DECR:
      //noteVelocity = currentVelocity;
      _currentVelocity = ((unsigned int)_currentVelocity * 250) >> 8;
      if (restartVelocity) _currentVelocity = _maxVelocity;
      break;
  }
 
  // Send note on  
  while (noteNumber > 127) noteNumber -= 12; // to prevent MIDI overflow
  SendNoteOn(noteNumber, _currentVelocity);
  _currentNoteNumber = noteNumber;
}


///////// DEBUG

template <class T> void ArpEngine::Print(T t) {
  if (_debugPort) _debugPort->print(t);
}
template <class T> void ArpEngine::Print(T t, int format) {
  if (_debugPort) _debugPort->print(t, format);
}
template <class T> void ArpEngine::PrintLn(T t) {
  if (_debugPort) _debugPort->println(t);
}
template <class T> void ArpEngine::PrintLn(T t, int format) {
  if (_debugPort) _debugPort->println(t, format);
}
void ArpEngine::PrintList(byte* list, int length) {
  Print('[');
  for (int i = 0; i < length; i++) {
    if (i > 0) Print(',');
    Print(list[i]);
  }
  Print(']');
}
void ArpEngine::PrintNoteList() {
  Print("  "); PrintList(_noteNumbersSorted, _noteCount);
  Print("  _noteCount: "); Print(_noteCount);
  PrintLn("");
}
void ArpEngine::PrintEventSchedule() {
  Print("Now: "); Print(_midiSync ? _pulseCounter : _now);
  Print("  Next event on/off: ");
  Print(_midiSync ? _nextOnEventAtPulse : _nextOnEventAt);
  Print(" ");
  Print(_midiSync ? _nextOffEventAtPulse : _nextOffEventAt);
  PrintLn("");
}


///////// PUBLIC

void ArpEngine::Run(ulong now)
{
  _now = now;

  // Run events
  if (_isEnabled && _noteCount > 0)
  {
    if (_midiSync)
    {
      if (_pulseCounter >= _nextOffEventAtPulse && _nextOffEventAtPulse > 0) {
        HandleArpeggiatorOffEvent();
        _nextOffEventAtPulse = 0;
        PrintEventSchedule();
      }
      if (_pulseCounter >= _nextOnEventAtPulse) {
        HandleArpeggiatorOnEvent();
        _nextOnEventAtPulse = _pulseCounter + _noteIntervalPulses;
        if (_snapToBeat) {
          // Snap next event to nearest multiple of the note interval
          _nextOnEventAtPulse += _noteIntervalPulses/2; // add half interval...
          _nextOnEventAtPulse -= _nextOnEventAtPulse % _noteIntervalPulses; // ... then truncate (round down)
        }
        _nextOffEventAtPulse = MIN(
          _pulseCounter + _gateLengthPulses, _nextOnEventAtPulse);
        
        PrintEventSchedule();
      }
    }
    else
    {
      if (_now >= _nextOffEventAt && _nextOffEventAt > 0) {
        HandleArpeggiatorOffEvent();
        _nextOffEventAt = 0;
        PrintEventSchedule();
      }
      if (_now >= _nextOnEventAt) {
        HandleArpeggiatorOnEvent();
        _nextOnEventAt = _now + _delayMs;
        _nextOffEventAt = _now + _delayMsGate;
        PrintEventSchedule();
      }
    }
  }

  // Handle new MIDI data
  while (_midiPort->available() > 0) {
    byte data = _midiPort->read();
    HandleMidiData(data);
    if (onMidiIn != NULL) onMidiIn();
  }
  // Handle sync data
  while (_syncPort->available() > 0) {
    byte data = _syncPort->read();
    HandleSyncData(data);
  }

  // onBeat event
  if (_midiSync) {
    if (_pulseCounter >= _nextBeatEventAtPulse) {
      _nextBeatEventAtPulse = _pulseCounter + _noteIntervalPulses;
      if (onBeat != NULL) onBeat();
    }
  }
  else {
    if (_now >= _nextBeatEventAt) {
      _nextBeatEventAt = _now + _delayMs;
      if (onBeat != NULL) onBeat();
    }
  }
}

void ArpEngine::SetEnabled(bool enabled)
{
  _isEnabled = enabled;
  if (_isEnabled)
  {
    if (_noteCount > 0) // convert current chord to arpeggio
    {
      for (byte i = 0; i < _noteCount; i++) {
        SendNoteOff(_noteNumbers[i]);
      }  
      InitArpeggio();
    }
  }
  else
  {
    if (_noteCount > 0) // convert current arpeggio to chord
    {
      SendNoteOff(_currentNoteNumber);
      if (!_hold) {
        for (byte i = 0; i < _noteCount; i++) {
          SendNoteOn(_noteNumbers[i], _noteVelocities[i]);
        }
      }
    }
  }
}

void ArpEngine::SetHold(bool hold)
{
  _hold = hold;
  if (!_hold) {
    SendNoteOff(_currentNoteNumber);
    _noteCount = 0;
  }
}

void ArpEngine::SetTempo(int tempo)
{
  _tempo = tempo;
  _delayMs = ((ulong)60000/4)/tempo;
  _delayMsGate = _delayMs * _gate / 100;

  // Set note interval in pulses (for MIDI sync mode)
  int lengthMode = (tempo-MIN_TEMPO)*LENGTH_COUNT/(MAX_TEMPO-MIN_TEMPO);
  switch (lengthMode) {
    // 6 pulses per 1/16 note
    case LENGTH_WHOLE: _noteIntervalPulses = 96; break;
    case LENGTH_HALF: _noteIntervalPulses = 48; break;
    case LENGTH_THIRD: _noteIntervalPulses = 36; break;
    case LENGTH_QUARTER: _noteIntervalPulses = 24; break;
    case LENGTH_SIXTH: _noteIntervalPulses = 18; break;
    case LENGTH_EIGHTH: _noteIntervalPulses = 12; break;
    case LENGTH_TWELVTH: _noteIntervalPulses = 8; break;
    case LENGTH_SIXTEENTH: _noteIntervalPulses = 6; break;
    case LENGTH_TWENTYFOURTH: _noteIntervalPulses = 4; break;
    case LENGTH_THIRTYSECOND: default: _noteIntervalPulses = 3; break;
  }
  _gateLengthPulses = _noteIntervalPulses; // TODO: compute actual gate length

  Print("Tempo: "); Print(_tempo);
  Print(", _delayMs: "); Print(_delayMs);
  Print(", _delayMsGate: "); PrintLn(_delayMsGate);
}

void ArpEngine::SetGate(int gateLength) // 0..100 (%)
{
  _gate = gateLength;
  _delayMsGate = _delayMs * _gate / 100;

  Print("Gate: "); Print(_gate);
  Print(", _delayMs: "); Print(_delayMs);
  Print(", _delayMsGate: "); PrintLn(_delayMsGate);
}

void ArpEngine::SetMidiSync(bool midiSyncEnabled)
{
  // NOTE: To ensure everything is set up correctly,
  // we temporarily turn the arpeggiator off
  // if running
  bool wasEnabled = _isEnabled;
  if (_isEnabled) SetEnabled(false);
  _midiSync = midiSyncEnabled;
  if (wasEnabled) SetEnabled(true);
  Print("MIDI Sync: ");
  Print(midiSyncEnabled ? "on" : "off");
}

void ArpEngine::SetMode(int mode)
{
  _mode = mode;
  
  Print("Mode: ");
  switch (_mode)
  {
    case MODE_UP: PrintLn("Up"); break;
    case MODE_DOWN: PrintLn("Down"); break;
    case MODE_UP_DOWN: PrintLn("Up/Down"); break;
    //case MODE_ORDER: PrintLn("Order"); break;
    //case MODE_RANDOM1: PrintLn("Random1"); break;
    //case MODE_RANDOM2: PrintLn("Random2"); break;
    case MODE_RANDOM: PrintLn("Random"); break;
    default: PrintLn("Mode error."); break;
  }
}

void ArpEngine::SetVelocityMode(int velocityMode)
{
  _velMode = velocityMode;
}

void ArpEngine::SetRange(int extraOctaves)
{
  _range = extraOctaves;
}

// int ArpEngine::GetBeatDelayMs()
// {
//   // TODO: add MIDI sync support
//   return _delayMs;
// }