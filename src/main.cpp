#include <Arduino.h>
#include "Potentiometer.h"
#include "Button.h"
#include "LedFlasher.h"
#include "ArpEngine.h"

// Serial pins
static const int MIDI_IN_PIN = 1;
static const int MIDI_OUT_PIN = 0;
static const int MIDI_SYNC_IN_PIN = 5;

// ADC pins
static const int TEMPO_PIN = 27;
static const int TEMPO_ADC_CHANNEL = A1;
static const int GATE_PIN = 26;
static const int GATE_ADC_CHANNEL = A0;

// Button pins
static const int SYNC_PIN = 22;
static const int MODE_PIN = 9;
static const int OCT_PIN = 14;
static const int ONOFF_PIN = 8;
static const int HOLD_PIN = 2; // press: hold on/off, hold: chords mode

// LED pins
static const int MIDI_IN_LED_PIN = LED_BUILTIN;
static const int TEMPO_LED_PIN = 21;
static const int SYNC_LED_PIN = 20;
static const int MODE_UP_LED_PIN = 13;
static const int MODE_DOWN_LED_PIN = 12;
static const int MODE_UP_DOWN_LED_PIN = 11;
static const int MODE_RANDOM_LED_PIN = 10;
static const int OCT1_LED_PIN = 15;
static const int OCT2_LED_PIN = 16;
static const int OCT3_LED_PIN = 17;
static const int OCT4_LED_PIN = 18;
static const int OCT5_LED_PIN = 19;
static const int ONOFF_LED_PIN = 7;
static const int CHORDS_LED_PIN = 3;
static const int HOLD_LED_PIN = 6;

/*
// This is the old pin configuration from the breadboard prototype.
// (the pins were later changed to optimize PCB layout)
// I'm keeping this around temporarily since I don't want
// to rip all those wires up...

// ADC pins
static const int TEMPO_PIN = 26;
static const int TEMPO_ADC_CHANNEL = A0;
static const int GATE_PIN = 27;
static const int GATE_ADC_CHANNEL = A1;

// Button pins
static const int SYNC_PIN = 15;
static const int MODE_PIN = 16;
static const int OCT_PIN = 17;
static const int ONOFF_PIN = 18; // press: on/off, hold: chords mode
static const int HOLD_PIN = 19;

// LED pins
static const int MIDI_IN_LED_PIN = LED_BUILTIN;
static const int TEMPO_LED_PIN = 20;
static const int SYNC_LED_PIN = 2;
static const int MODE_UP_LED_PIN = 3;
static const int MODE_DOWN_LED_PIN = 21;
static const int MODE_UP_DOWN_LED_PIN = 22;
static const int MODE_RANDOM_LED_PIN = 6;
static const int OCT1_LED_PIN = 7;
static const int OCT2_LED_PIN = 8;
static const int OCT3_LED_PIN = 9;
static const int OCT4_LED_PIN = 10;
static const int OCT5_LED_PIN = 11;
static const int ONOFF_LED_PIN = 12;
static const int CHORDS_LED_PIN = 13;
static const int HOLD_LED_PIN = 14;
*/

// Constants
static const int BUTTON_DEBOUNCE_MS = 30;
static const int BUTTON_HELD_MS = 700;

// state
ulong now; // current synchronized timestamp (ms)
bool sync = false;
bool enabled = false;
bool hold = false;
int oct = 0; // extra octaves
int type = ArpEngine::MODE_UP;
int tempo = 100;
int gate = 100;
int status_led = false;


////////// I/O

Button syncButton = Button(SYNC_PIN, BUTTON_DEBOUNCE_MS);
Button modeButton = Button(MODE_PIN, BUTTON_DEBOUNCE_MS);
Button octButton = Button(OCT_PIN, BUTTON_DEBOUNCE_MS);
Button onOffButton = Button(ONOFF_PIN, BUTTON_DEBOUNCE_MS, BUTTON_HELD_MS);
Button holdButton = Button(HOLD_PIN, BUTTON_DEBOUNCE_MS);

LedFlasher tempoLed = LedFlasher(TEMPO_LED_PIN, 40);
LedFlasher midiInLed = LedFlasher(MIDI_IN_LED_PIN, 20);

Potentiometer tempoPot = Potentiometer(TEMPO_ADC_CHANNEL,
  30, 990, ArpEngine::MIN_TEMPO, ArpEngine::MAX_TEMPO);
Potentiometer gatePot = Potentiometer(GATE_ADC_CHANNEL,
  30, 990, ArpEngine::MIN_GATE, ArpEngine::MAX_GATE);

ArpEngine arpEngine = ArpEngine(&Serial1, &Serial2, &Serial);


////////// Helpers

void setModeLed(int pin) {
  digitalWrite(MODE_UP_LED_PIN, LOW);
  digitalWrite(MODE_DOWN_LED_PIN, LOW);
  digitalWrite(MODE_UP_DOWN_LED_PIN, LOW);
  digitalWrite(MODE_RANDOM_LED_PIN, LOW);
  digitalWrite(pin, HIGH);
}
void setOctLed(int pin) {
  digitalWrite(OCT1_LED_PIN, LOW);
  digitalWrite(OCT2_LED_PIN, LOW);
  digitalWrite(OCT3_LED_PIN, LOW);
  digitalWrite(OCT4_LED_PIN, LOW);
  digitalWrite(OCT5_LED_PIN, LOW);
  digitalWrite(pin, HIGH);
}


////////// Event handlers

void syncButtonDown() {
  sync = ! sync;
  digitalWrite(SYNC_LED_PIN, sync);
  arpEngine.SetMidiSync(sync);
  Serial.println(sync ? "Sync: On" : "Sync: Off");
}
void modeButtonDown() {
  if (++type >= ArpEngine::MODE_COUNT) type = 0;
  arpEngine.SetMode(type);
  switch (type) {
    case ArpEngine::MODE_UP: setModeLed(MODE_UP_LED_PIN); break;
    case ArpEngine::MODE_DOWN: setModeLed(MODE_DOWN_LED_PIN); break;
    case ArpEngine::MODE_UP_DOWN: setModeLed(MODE_UP_DOWN_LED_PIN); break;
    case ArpEngine::MODE_RANDOM: setModeLed(MODE_RANDOM_LED_PIN); break;
  }
  Serial.print("Mode: ");
  Serial.println(type);
}
void octButtonDown() {
  if (++oct > 4) oct = 0;
  switch (oct) {
    case 0: setOctLed(OCT1_LED_PIN); break;
    case 1: setOctLed(OCT2_LED_PIN); break;
    case 2: setOctLed(OCT3_LED_PIN); break;
    case 3: setOctLed(OCT4_LED_PIN); break;
    case 4: setOctLed(OCT5_LED_PIN); break;
  }
  arpEngine.SetRange(oct);
  Serial.print("Oct: ");
  Serial.println(oct);
}
void onOffButtonDown() {
}
void onOffButtonUpNotHeld() {
  enabled = ! enabled;
  digitalWrite(ONOFF_LED_PIN, enabled);
  arpEngine.SetEnabled(enabled);
  Serial.println(enabled ? "On" : "Off");
}
void onOffButtonHeld() {
  // TODO: toggle chords mode
}
void holdButtonDown() {
  hold = ! hold;
  digitalWrite(HOLD_LED_PIN, hold);
  arpEngine.SetHold(hold);
  Serial.println(hold ? "Hold: On" : "Hold: Off");
}
void onMidiIn() {
  midiInLed.flash(now);
}
void onBeat() {
  tempoLed.flash(now);
}


////////// Initialization

void setup() {
  now = millis();

  Serial.begin(115200); // rate doesn't matter for USB
  Serial.println("Starting...");

  // MIDI In/Out (Serial1)
  pinMode(MIDI_OUT_PIN, OUTPUT); // UART0 TX (Serial1)
  pinMode(MIDI_IN_PIN, INPUT); // UART0 RX (Serial1)
  Serial1.setTX(MIDI_OUT_PIN);
  Serial1.setRX(MIDI_IN_PIN);
  Serial1.setFIFOSize(256);
  Serial1.begin(31250);
  // MIDI Sync In (Serial2)
  pinMode(MIDI_SYNC_IN_PIN, INPUT); // UART1 RX (Serial2)
  Serial2.setRX(MIDI_SYNC_IN_PIN);
  Serial2.setFIFOSize(256);
  Serial2.begin(31250);

  // force SMPS PWM mode for all loads
  // for reduced ripple and thus improved ADC performance
  // (see RPi Pico datasheet)
  pinMode(23, OUTPUT);
  analogWrite(23, HIGH);

  // pots
  pinMode(TEMPO_PIN, INPUT);
  pinMode(GATE_PIN, INPUT);
  analogReadResolution(10); // TODO: Switch to 12-bit ADC

  // buttons
  pinMode(SYNC_PIN, INPUT_PULLUP);
  pinMode(MODE_PIN, INPUT_PULLUP);
  pinMode(OCT_PIN, INPUT_PULLUP);
  pinMode(ONOFF_PIN, INPUT_PULLUP);
  pinMode(HOLD_PIN, INPUT_PULLUP);
  
  // leds
  pinMode(MIDI_IN_LED_PIN, OUTPUT);
  pinMode(TEMPO_LED_PIN, OUTPUT);
  pinMode(SYNC_LED_PIN, OUTPUT);
  pinMode(MODE_UP_LED_PIN, OUTPUT);
  pinMode(MODE_DOWN_LED_PIN, OUTPUT);
  pinMode(MODE_UP_DOWN_LED_PIN, OUTPUT);
  pinMode(MODE_RANDOM_LED_PIN, OUTPUT);
  pinMode(OCT1_LED_PIN, OUTPUT);
  pinMode(OCT2_LED_PIN, OUTPUT);
  pinMode(OCT3_LED_PIN, OUTPUT);
  pinMode(OCT4_LED_PIN, OUTPUT);
  pinMode(OCT5_LED_PIN, OUTPUT);
  pinMode(ONOFF_LED_PIN, OUTPUT);
  pinMode(CHORDS_LED_PIN, OUTPUT);
  pinMode(HOLD_LED_PIN, OUTPUT);
  setModeLed(MODE_UP_LED_PIN); // initial value
  setOctLed(OCT1_LED_PIN); // initial value

  // Initialize event handlers
  syncButton.buttonDown = syncButtonDown;
  modeButton.buttonDown = modeButtonDown;
  octButton.buttonDown = octButtonDown;
  onOffButton.buttonDown = onOffButtonDown;
  onOffButton.buttonUpNotHeld = onOffButtonUpNotHeld;
  onOffButton.buttonHeld = onOffButtonHeld;
  holdButton.buttonDown = holdButtonDown;
  arpEngine.onMidiIn = onMidiIn;
  arpEngine.onBeat = onBeat;
}


////////// Main loop

void loop()
{
  // single reading to ensure everything is
  // synchronized
  now = millis();

  // scan buttons
  syncButton.scan(now);
  modeButton.scan(now);
  octButton.scan(now);
  onOffButton.scan(now);
  holdButton.scan(now);

  // scan pots
  tempoPot.sample();
  if (tempoPot.hasNewOutputValue()) {
    tempo = tempoPot.readOutputValue();
    arpEngine.SetTempo(tempo);
  }
  gatePot.sample();
  if (gatePot.hasNewOutputValue()) {
    gate = gatePot.readOutputValue();
    arpEngine.SetGate(gate);
  }
  
  // update blinky LEDs
  tempoLed.run(now);
  midiInLed.run(now);
  
  // run arpeggiator and handle MIDI input
  arpEngine.Run(now);
}
