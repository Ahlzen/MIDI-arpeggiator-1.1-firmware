#include "common.h"
#include "ArpEngine.h"


ArpEngine::ArpEngine(
  HardwareSerial* midiPort,
  HardwareSerial* debugPort)
{
  _midiPort = midiPort;
  _debugPort = debugPort;
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
}

void ArpEngine::SendNoteOff(byte noteNumber) {
  _midiPort->write(0x90 + _midiChannel);
  _midiPort->write(noteNumber);
  _midiPort->write((uint8_t)0); // velocity 0 = note off
  //Print("SendNoteOff: "); PrintLn(noteNumber);
}

void ArpEngine::ForwardMidiData(byte data) {
  _midiPort->write(data);
}

void ArpEngine::ForwardMidiData2Byte() {
  _midiPort->write(_midiStatus + _midiChannel);
  _midiPort->write(_midiData1);
}

void ArpEngine::ForwardMidiData3Byte() {
  _midiPort->write(_midiStatus + _midiChannel);
  _midiPort->write(_midiData1);
  _midiPort->write(_midiData2);
}



///////// MIDI INPUT

void ArpEngine::HandleNoteOn(ulong now)
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
      InitArpeggio(now);
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

void ArpEngine::HandleMidiData(ulong now, byte data)
{
  if (_midiState == MIDI_IN_SYSEX) {
    ForwardMidiData(data);
    if (data == 0xF7) {
      // end of SysEx
      _midiState = MIDI_WAITING_FOR_STATUS_OR_DATA1;
      _midiStatus = 0;
    }
    return;
  }
  
  if (data & 0b10000000) {
    // status byte
    if (data == 0xF0) {
      // start of SysEx
      ForwardMidiData(data);
      _midiState = MIDI_IN_SYSEX;
    } else if ((data & 0xf0) == 0xf0) {
      // system message: pass through
      ForwardMidiData(data);
      _midiStatus = 0;
    } else {
      // channel message
      _midiStatus = data & 0xf0;
      _midiChannel = data & 0x0f;
      _midiState = MIDI_WAITING_FOR_STATUS_OR_DATA1;
    }
  } else {
    // data byte
    switch (_midiState) {
      case MIDI_WAITING_FOR_STATUS_OR_DATA1: // first data byte
        _midiData1 = data;
        switch (_midiStatus) {
          case 0x80: // note off
          case 0x90: // note on
          case 0xa0: // poly key pressure
          case 0xb0: // control change
          case 0xe0: // pitch bend
            _midiState = MIDI_WAITING_FOR_DATA2;
            break;
          case 0xc0: // program change
          case 0xd0: // channel pressure
            ForwardMidiData2Byte(); // pass-through
            break;
        }
        break;
      case MIDI_WAITING_FOR_DATA2: // second data byte
        _midiData2 = data;
        switch (_midiStatus) {
          case 0x80: // note off
            HandleNoteOff();
            break;
          case 0x90: // note on
            HandleNoteOn(now);
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

void ArpEngine::InitArpeggio(ulong now)
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
  _nextOnEventAt = now + _delayMs;
  _nextOffEventAt = now + _delayMsGate;
  PrintEventSchedule(now);
  _maxVelocity = _noteVelocities[0];
}

void ArpEngine::HandleArpeggiatorOffEvent()
{
  SendNoteOff(_currentNoteNumber);
  _lastOctave = _currentOctave;
  _lastNoteIndex = _currentNoteIndex; 
}

void ArpEngine::HandleArpeggiatorOnEvent(ulong now)
{
  bool restartVelocity = false;
  
  // Advance one step (this is mode dependent)
  switch (_mode) {
    case MODE_UP:
    case MODE_ORDER:
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
    case MODE_RANDOM1:
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
    case MODE_RANDOM2:
    {
      if (_noteCount < 2 && _range == 0) break; // single note
      
      // Try to pick both a different note and octave than last time
      if (_noteCount > 1) while (_currentNoteIndex == _lastNoteIndex)
        _currentNoteIndex = random(_noteCount);
      if (_range > 0) while (_currentOctave == _lastOctave)
        _currentOctave = random(_range+1);
      break;
    }
    default: break;
  }
  
  // Find next note
  
  byte* noteNumberList = _noteNumbersSorted;
  byte* noteVelocityList = _noteVelocitiesSorted;
  if (_mode == MODE_ORDER) {
    // use ordered list instead of sorted
    noteNumberList = _noteNumbers;
    noteVelocityList = _noteVelocities;
  }
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
void ArpEngine::PrintEventSchedule(ulong now) {
  Print("Now: "); Print(now);
  Print("  Next event on/off: "); Print(_nextOnEventAt);
  Print(" "); Print(_nextOffEventAt);
  PrintLn("");
}


///////// PUBLIC

void ArpEngine::Run(ulong now)
{
  // Run events
  if (_isEnabled && _noteCount > 0)
  {
    if (now >= _nextOffEventAt && _nextOffEventAt > 0) {
      HandleArpeggiatorOffEvent();
      _nextOffEventAt = 0;
      PrintEventSchedule(now);
    }
    if (now >= _nextOnEventAt) {
      HandleArpeggiatorOnEvent(now);
      _nextOnEventAt = now + _delayMs;
      _nextOffEventAt = now + _delayMsGate;
      PrintEventSchedule(now);
    }
  }

  // Handle new MIDI data
  while (_midiPort->available() > 0) {
    byte data = _midiPort->read();
    HandleMidiData(now, data);
  }
}

void ArpEngine::SetEnabled(ulong now, bool enabled)
{
  _isEnabled = enabled;
  if (_isEnabled)
  {
    if (_noteCount > 0) // convert current chord to arpeggio
    {
      for (byte i = 0; i < _noteCount; i++) {
        SendNoteOff(_noteNumbers[i]);
      }  
      InitArpeggio(now);
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

void ArpEngine::SetTempo(int tempo) // 30..300 (BPM)
{
  _tempo = tempo;
  _delayMs = ((ulong)60000/4)/tempo;
  _delayMsGate = _delayMs * _gate / 100;

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

void ArpEngine::SetMode(int mode)
{
  _mode = mode;
  
  Print("Mode: ");
  switch (_mode)
  {
    case MODE_UP: PrintLn("Up"); break;
    case MODE_DOWN: PrintLn("Down"); break;
    case MODE_UP_DOWN: PrintLn("Up/Down"); break;
    case MODE_ORDER: PrintLn("Order"); break;
    case MODE_RANDOM1: PrintLn("Random1"); break;
    case MODE_RANDOM2: PrintLn("Random2"); break;
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

int ArpEngine::GetBeatDelayMs()
{
  return _delayMs;
}