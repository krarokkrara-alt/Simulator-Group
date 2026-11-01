/* Mega 2560 bench waveform bring-up ONLY.
 * D11: 36-slot / 2 missing CKP logic test. D12: optional CAM test pulse.
 * D5/D6/D7: TPS/MAP/MAF test PWM, NOT DAC voltages or ECU-ready outputs.
 * Both pins are high impedance during reset/bootloader. Interface must default off.
 * Not an ECU-qualified 89661-02860 profile. No CAM phase is assumed.
 * Use an oscilloscope/logic analyzer; never wire directly to ECU NE/G.
 * Serial 115200: RPM 800, START, STOP, STATUS.
 * START must be renewed within 5 seconds (main-loop timeout, not hardware protection).
 */
#include <Arduino.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <stdlib.h>
#include <string.h>

#if !defined(__AVR_ATmega2560__)
#error Select Arduino Mega 2560
#endif

volatile uint8_t halfSlot = 0;
volatile bool running = false;
volatile bool camEnabled = false;
volatile uint8_t camPhase = 0;
volatile uint8_t camWidth = 1;
const uint8_t sensorPins[3] = {5, 6, 7}; // Timer3A, Timer4A, Timer4B.
uint16_t sensorMv[3] = {0, 0, 0};
unsigned long sensorUpdatedAt[3] = {0, 0, 0};
uint16_t rpm = 800;
unsigned long lastStart = 0;
char line[48];
uint8_t used = 0;
bool overflowed = false;

ISR(TIMER1_COMPA_vect) {
  // Mega D11 = PB5. 68 active half-slots + 4 missing half-slots.
  uint8_t crankSlot = halfSlot % 72;
  if (running && crankSlot < 68 && !(crankSlot & 1)) PORTB |= _BV(PB5);
  else PORTB &= ~_BV(PB5);
  // Half-slot zero starts CKP tooth zero. CAM phase is UNVERIFIED.
  uint8_t camOffset = (halfSlot + 144U - camPhase) % 144U;
  if (running && camEnabled && camOffset < camWidth) PORTB |= _BV(PB6);
  else PORTB &= ~_BV(PB6);
  halfSlot = (halfSlot + 1) % 144;
}

void sensorsOff() {
  for (uint8_t i = 0; i < 3; ++i) {
    analogWrite(sensorPins[i], 0);
    sensorMv[i] = 0;
    sensorUpdatedAt[i] = 0;
  }
}

void stopOutput() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    running = false;
    TIMSK1 &= ~_BV(OCIE1A);
    PORTB &= ~(_BV(PB5) | _BV(PB6));
    halfSlot = 0;
  }
}

void stopAll() {
  stopOutput();
  sensorsOff();
}

const __FlashStringHelper *sensorName(uint8_t index) {
  if (index == 0) return F("TPS");
  if (index == 1) return F("MAP");
  return F("MAF");
}

void sensorCommand(char *args) {
  if (!strcmp(args, "OFF")) {
    sensorsOff(); Serial.println(F("SENSOR OFF")); return;
  }
  char *space = strchr(args, ' ');
  if (!space) { Serial.println(F("ERROR SENSOR name value")); return; }
  *space = 0;
  int8_t index = !strcmp(args, "TPS") ? 0 : !strcmp(args, "MAP") ? 1 :
                 !strcmp(args, "MAF") ? 2 : -1;
  char *end;
  long mv = strtol(space + 1, &end, 10);
  if (index < 0 || end == space + 1 || *end || mv < 0 || mv > 5000) {
    Serial.println(F("ERROR SENSOR TPS|MAP|MAF 0..5000")); return;
  }
  sensorMv[index] = (uint16_t)mv;
  uint8_t duty = ((uint32_t)mv * 255UL + 2500UL) / 5000UL;
  analogWrite(sensorPins[index], duty);
  sensorUpdatedAt[index] = millis();
  Serial.print(F("ACK SENSOR ")); Serial.print(sensorName(index));
  Serial.print(' '); Serial.print(mv); Serial.println(F(" mV"));
  Serial.println(F("TEST PWM ONLY; nominal average at 5V; NOT DAC/ECU READY"));
}

void camCommand(char *args) {
  if (running) { Serial.println(F("ERROR STOP before changing CAM")); return; }
  if (!strcmp(args, "OFF")) {
    camEnabled = false; digitalWrite(12, LOW);
    Serial.println(F("ACK CAM OFF")); return;
  }
  char *end;
  long phase = strtol(args, &end, 10);
  if (end == args || *end != ' ') {
    Serial.println(F("ERROR CAM phase width")); return;
  }
  char *widthArg = end + 1;
  long width = strtol(widthArg, &end, 10);
  if (end == widthArg || *end || phase < 0 || phase > 143 || width < 1 || width > 12) {
    Serial.println(F("ERROR CAM phase 0..143 width 1..12")); return;
  }
  camPhase = phase; camWidth = width; camEnabled = true;
  Serial.print(F("ACK CAM ")); Serial.print(phase); Serial.print(' ');
  Serial.println(width);
  Serial.println(F("TEST SINGLE PULSE/720; PHASE UNVERIFIED; NOT ECU PROFILE"));
}

void setSpeed(uint16_t value) {
  // 16 MHz / 8 = 2 MHz; 72 half-slots per revolution.
  uint16_t top = (120000000UL + (uint32_t)value * 36UL) /
                 ((uint32_t)value * 72UL) - 1;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    OCR1A = top;
    TCNT1 = 0;
  }
  rpm = value;
}

void command() {
  line[used] = 0;
  if (!strcmp(line, "STOP")) {
    stopAll(); Serial.println(F("STOPPED"));
  } else if (!strcmp(line, "START")) {
    lastStart = millis();
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      if (!running) {
        halfSlot = 0; TCNT1 = 0;
        TIFR1 = _BV(OCF1A);
        running = true; TIMSK1 |= _BV(OCIE1A);
      }
    }
    Serial.println(F("BENCH TEST; renew START within 5s; PHASE UNVERIFIED"));
  } else if (!strncmp(line, "RPM ", 4)) {
    char *end;
    long value = strtol(line + 4, &end, 10);
    if (end == line + 4 || *end || value < 100 || value > 6000) {
      Serial.println(F("ERROR RPM range 100..6000"));
    } else if (running) {
      Serial.println(F("ERROR STOP before changing RPM"));
    } else {
      setSpeed((uint16_t)value); Serial.println(F("RPM SET"));
    }
  } else if (!strncmp(line, "SENSOR ", 7)) {
    sensorCommand(line + 7);
  } else if (!strncmp(line, "CAM ", 4)) {
    camCommand(line + 4);
  } else if (!strcmp(line, "CONFIG")) {
    Serial.print(F("CONFIG CAM "));
    if (!camEnabled) Serial.println(F("OFF"));
    else {
      Serial.print(camPhase); Serial.print(' '); Serial.print(camWidth);
      Serial.println(F(" PHASE UNVERIFIED"));
    }
    for (uint8_t i = 0; i < 3; ++i) {
      Serial.print(F("CONFIG SENSOR ")); Serial.print(sensorName(i));
      Serial.print(' '); Serial.print(sensorMv[i]); Serial.println(F(" mV TEST PWM"));
    }
  } else if (!strcmp(line, "STATUS")) {
    Serial.print(running ? F("RUN ") : F("STOP "));
    Serial.print(rpm);
    Serial.println(camEnabled ? F(" RPM; CAM TEST ENABLED; ECU UNVERIFIED") :
                                F(" RPM; CAM DISABLED; ECU UNVERIFIED"));
  } else Serial.println(F("Commands: RPM | START | STOP | STATUS | CONFIG | SENSOR | CAM"));
}

void setup() {
  pinMode(11, OUTPUT); pinMode(12, OUTPUT);
  digitalWrite(11, LOW); digitalWrite(12, LOW);
  for (uint8_t i = 0; i < 3; ++i) pinMode(sensorPins[i], OUTPUT);
  TCCR1A = 0; TCCR1B = _BV(WGM12) | _BV(CS11);
  stopAll(); setSpeed(rpm);
  Serial.begin(115200);
  Serial.println(F("Mega bench test only. Output stopped. CAM disabled."));
}

void loop() {
  if (running && (unsigned long)(millis() - lastStart) >= 5000UL) {
    stopAll(); Serial.println(F("STOP: START timeout"));
  }
  for (uint8_t i = 0; i < 3; ++i) {
    if (sensorMv[i] && (unsigned long)(millis() - sensorUpdatedAt[i]) >= 5000UL) {
      analogWrite(sensorPins[i], 0); sensorMv[i] = 0;
      Serial.print(F("SENSOR TIMEOUT ")); Serial.println(sensorName(i));
    }
  }
  // Bound each pass so an incoming stream cannot starve timeout handling.
  for (uint8_t n = 0; n < 32 && Serial.available(); ++n) {
    char c = Serial.read();
    if (c == '\r' || c == '\n') {
      if (overflowed) Serial.println(F("ERROR line too long"));
      else if (used) command();
      used = 0; overflowed = false;
    } else if (!overflowed) {
      if (used < sizeof(line) - 1) line[used++] = c;
      else { overflowed = true; stopAll(); }
    }
  }
}
