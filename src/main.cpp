#include <Arduino.h>
#include "Potentiometer.h"
#include "Button.h"
#include "LedFlasher.h"
#include "ArpEngine.h"

static const int TEMPO_PIN = 26;
static const int TEMPO_ADC_CHANNEL = A0;
static const int GATE_PIN = 27;
static const int GATE_ADC_CHANNEL = A1;
static const int TYPE_PIN = 16;
static const int OCT_PIN = 17;
static const int ONOFF_PIN = 18;
static const int ONOFF_LED_PIN = 12;
static const int HOLD_LED_PIN = 14;

static const int TYPE_UP = 0;
static const int TYPE_DOWN = 1;
static const int TYPE_UP_DOWN = 2;
static const int TYPE_RANDOM = 3;
static const int TYPE_COUNT = 4; // highest type + 1

static const int BUTTON_DEBOUNCE_MS = 30;
static const int BUTTON_HELD_MS = 700;


// state
bool enabled = false;
bool hold = false;
int oct = 0; // extra octaves
int type = ArpEngine::MODE_UP;
int tempo = 100;
int gate = 100;

int status_led = false;

long nextBlinkAt = 0; // blink timer



////////// I/O

Button typeButton = Button(TYPE_PIN, BUTTON_DEBOUNCE_MS);
Button octButton = Button(OCT_PIN, BUTTON_DEBOUNCE_MS);
Button onOffButton = Button(ONOFF_PIN, BUTTON_DEBOUNCE_MS, BUTTON_HELD_MS);
Potentiometer tempoPot = Potentiometer(TEMPO_ADC_CHANNEL, 30, 990, 30, 300);
Potentiometer gatePot = Potentiometer(GATE_ADC_CHANNEL, 30, 990, 0, 100);
ArpEngine arpEngine = ArpEngine(&Serial1, &Serial);
LedFlasher tempoLed = LedFlasher(LED_BUILTIN, 40);


////////// Event handlers

void typeButtonDown() {
  if (++type >= ArpEngine::MODE_COUNT) type = 0;
  arpEngine.SetMode(type);
  Serial.print("Mode: ");
  Serial.println(type);
}
void octButtonDown() {
  if (++oct > 4) oct = 0;
  arpEngine.SetRange(oct);
  Serial.print("Oct: ");
  Serial.println(oct);
}
void onOffButtonDown() {
}
void onOffButtonUpNotHeld() {
    enabled = ! enabled;
  digitalWrite(ONOFF_LED_PIN, enabled);
  arpEngine.SetEnabled(millis(), enabled);
  Serial.println(enabled ? "On" : "Off");
}
void onOffButtonHeld() {
  hold = ! hold;
  digitalWrite(HOLD_LED_PIN, hold);
  arpEngine.SetHold(hold);
  Serial.println(hold ? "Hold: On" : "Hold: Off");
}


////////// Initialization

void setup() {
  Serial.begin(115200); // rate doesn't matter for USB
  Serial.println("Starting...");

  // MIDI In/Out (serial1)
  pinMode(0, OUTPUT); // UART0 TX (Serial1)
  pinMode(1, INPUT); // UART0 RX (Serial1)
  Serial1.setTX(0);
  Serial1.setRX(1);
  Serial1.setFIFOSize(256);
  Serial1.begin(31250);

  // force SMPS PWM mode for all loads
  // for reduced ripple and thus improved ADC performance
  // (see RPi Pico datasheet)
  pinMode(23, OUTPUT);
  analogWrite(23, HIGH);

  // buttons
  pinMode(TYPE_PIN, INPUT_PULLUP);
  pinMode(OCT_PIN, INPUT_PULLUP);
  pinMode(ONOFF_PIN, INPUT_PULLUP);

  // pots
  pinMode(TEMPO_PIN, INPUT);
  pinMode(GATE_PIN, INPUT);
  analogReadResolution(10); // TODO: Switch to 12-bit ADC

  // leds
  pinMode(LED_BUILTIN, OUTPUT); // status LED
  pinMode(ONOFF_LED_PIN, OUTPUT);
  pinMode(HOLD_LED_PIN, OUTPUT);

  // Initialize event handlers
  typeButton.buttonDown = typeButtonDown;
  octButton.buttonDown = octButtonDown;
  onOffButton.buttonDown = onOffButtonDown;
  onOffButton.buttonUpNotHeld = onOffButtonUpNotHeld;
  onOffButton.buttonHeld = onOffButtonHeld;

  nextBlinkAt = millis();
}


////////// Main loop

void loop()
{
  long now = millis();

  // scan buttons
  typeButton.scan(now);
  octButton.scan(now);
  onOffButton.scan(now);

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
  
  // run "heartbeat" blink
  // if (now - last_blink > 1000) {
  //   status_led = ! status_led;
  //   digitalWrite(LED_BUILTIN, status_led);
  //   last_blink = now;
  // }
  if (now >= nextBlinkAt) {
    tempoLed.flash(now);
    nextBlinkAt += arpEngine.GetBeatDelayMs() * 4;
  }
  tempoLed.run(now);
  
  // run arpeggiator and handle MIDI input
  arpEngine.Run(now);

}
