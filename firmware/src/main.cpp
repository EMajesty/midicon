#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal.h>

namespace {

constexpr uint32_t kConsoleBaud = 115200;
constexpr uint32_t kMidiBaud = 31250;
constexpr uint16_t kDebounceMs = 20;
constexpr uint16_t kAnalogIntervalMs = 25;
constexpr uint16_t kAnalogReportThreshold = 8;
constexpr uint16_t kLedIntervalMs = 250;

constexpr uint8_t kEncoderCount = 4;
constexpr uint8_t kEncoderAPins[kEncoderCount] = {5, 11, 17, 25};
constexpr uint8_t kEncoderBPins[kEncoderCount] = {7, 13, 21, 27};
constexpr uint8_t kEncoderButtonPins[kEncoderCount] = {3, 9, 15, 23};

constexpr uint8_t kFunctionCount = 4;
constexpr uint8_t kFunctionPins[kFunctionCount] = {33, 35, 37, 39};

constexpr uint8_t kMatrixSize = 4;
constexpr uint8_t kColumnPins[kMatrixSize] = {24, 26, 28, 30};
constexpr uint8_t kRowPins[kMatrixSize] = {41, 43, 45, 47};

constexpr uint8_t kAnalogCount = 6;
constexpr uint8_t kAnalogPins[kAnalogCount] = {A0, A2, A4, A6, A8, A10};
const char *const kAnalogNames[kAnalogCount] = {
    "Fader 1", "Fader 2", "Fader 3", "Fader 4",
    "Joystick 1 X", "Joystick 1 Y"};

constexpr uint8_t kLedPin = 32;
constexpr uint8_t kLedCount = 4;

LiquidCrystal lcd(36, 38, 40, 42, 44, 46);
Adafruit_NeoPixel leds(kLedCount, kLedPin, NEO_GRB + NEO_KHZ800);

struct DebouncedInput {
  bool stable = HIGH;
  bool sample = HIGH;
  uint32_t changedAt = 0;

  // Returns true only when a new stable state is accepted.
  bool update(bool newSample, uint32_t now) {
    if (newSample != sample) {
      sample = newSample;
      changedAt = now;
    }
    if (stable != sample && now - changedAt >= kDebounceMs) {
      stable = sample;
      return true;
    }
    return false;
  }
};

DebouncedInput encoderButtons[kEncoderCount];
DebouncedInput functionButtons[kFunctionCount];
DebouncedInput matrixButtons[kMatrixSize * kMatrixSize];

// Full-step quadrature transition table. Invalid/bouncing transitions add zero.
constexpr int8_t kQuadratureTable[16] = {
    0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
uint8_t encoderStates[kEncoderCount] = {};
int8_t encoderAccumulators[kEncoderCount] = {};
int32_t encoderPositions[kEncoderCount] = {};

uint16_t analogValues[kAnalogCount] = {};
uint32_t lastAnalogRead = 0;
uint32_t lastLedUpdate = 0;
uint8_t ledPhase = 0;

void showEvent(const char *kind, uint8_t number, const char *state) {
  Serial.print(kind);
  Serial.print(' ');
  Serial.print(number);
  Serial.print(": ");
  Serial.println(state);

  lcd.setCursor(0, 1);
  lcd.print("                    ");
  lcd.setCursor(0, 1);
  lcd.print(kind);
  lcd.print(' ');
  lcd.print(number);
  lcd.print(' ');
  lcd.print(state);
}

void initializeDebouncer(DebouncedInput &input, bool value) {
  input.stable = value;
  input.sample = value;
  input.changedAt = millis();
}

void scanDirectButtons(uint32_t now) {
  for (uint8_t i = 0; i < kEncoderCount; ++i) {
    if (encoderButtons[i].update(digitalRead(kEncoderButtonPins[i]), now)) {
      showEvent("Encoder PB", i + 1, encoderButtons[i].stable == LOW ? "pressed" : "released");
    }
  }

  for (uint8_t i = 0; i < kFunctionCount; ++i) {
    if (functionButtons[i].update(digitalRead(kFunctionPins[i]), now)) {
      showEvent("Function", i + 1, functionButtons[i].stable == LOW ? "pressed" : "released");
    }
  }
}

void scanEncoders() {
  for (uint8_t i = 0; i < kEncoderCount; ++i) {
    const uint8_t current = (digitalRead(kEncoderAPins[i]) << 1) |
                            digitalRead(kEncoderBPins[i]);
    const uint8_t transition = (encoderStates[i] << 2) | current;
    encoderStates[i] = current;
    encoderAccumulators[i] += kQuadratureTable[transition];

    if (encoderAccumulators[i] >= 4 || encoderAccumulators[i] <= -4) {
      const int8_t direction = encoderAccumulators[i] > 0 ? 1 : -1;
      encoderAccumulators[i] = 0;
      encoderPositions[i] += direction;
      Serial.print("Encoder ");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.println(encoderPositions[i]);
      showEvent("Encoder", i + 1, direction > 0 ? "+" : "-");
    }
  }
}

void scanMatrix(uint32_t now) {
  for (uint8_t row = 0; row < kMatrixSize; ++row) {
    pinMode(kRowPins[row], OUTPUT);
    digitalWrite(kRowPins[row], LOW);
    delayMicroseconds(5);

    for (uint8_t column = 0; column < kMatrixSize; ++column) {
      const uint8_t index = row * kMatrixSize + column;
      if (matrixButtons[index].update(digitalRead(kColumnPins[column]), now)) {
        showEvent("Matrix SW", index + 1,
                  matrixButtons[index].stable == LOW ? "pressed" : "released");
      }
    }

    // Write LOW before changing to INPUT so the pull-up stays disabled.
    digitalWrite(kRowPins[row], LOW);
    pinMode(kRowPins[row], INPUT);
  }
}

void scanAnalog(uint32_t now) {
  if (now - lastAnalogRead < kAnalogIntervalMs) return;
  lastAnalogRead = now;

  for (uint8_t i = 0; i < kAnalogCount; ++i) {
    const uint16_t value = analogRead(kAnalogPins[i]);
    const uint16_t difference = value > analogValues[i]
                                    ? value - analogValues[i]
                                    : analogValues[i] - value;
    if (difference >= kAnalogReportThreshold) {
      analogValues[i] = value;
      Serial.print(kAnalogNames[i]);
      Serial.print(": ");
      Serial.println(value);
    }
  }
}

void updateLeds(uint32_t now) {
  if (now - lastLedUpdate < kLedIntervalMs) return;
  lastLedUpdate = now;
  for (uint8_t i = 0; i < kLedCount; ++i) {
    const uint8_t hue = static_cast<uint8_t>(ledPhase + i * 64);
    leds.setPixelColor(i, leds.gamma32(leds.ColorHSV(static_cast<uint16_t>(hue) * 256)));
  }
  leds.show();
  ledPhase += 8;
}

void serviceMidi() {
  while (Serial1.available()) {
    const uint8_t value = Serial1.read();
    Serial1.write(value);  // Physical loop test: MIDI IN is echoed to MIDI OUT.
    Serial.print("MIDI RX: 0x");
    if (value < 0x10) Serial.print('0');
    Serial.println(value, HEX);
  }
}

}  // namespace

void setup() {
  Serial.begin(kConsoleBaud);
  Serial1.begin(kMidiBaud);

  for (uint8_t i = 0; i < kEncoderCount; ++i) {
    pinMode(kEncoderAPins[i], INPUT_PULLUP);
    pinMode(kEncoderBPins[i], INPUT_PULLUP);
    pinMode(kEncoderButtonPins[i], INPUT_PULLUP);
    encoderStates[i] = (digitalRead(kEncoderAPins[i]) << 1) |
                       digitalRead(kEncoderBPins[i]);
    initializeDebouncer(encoderButtons[i], digitalRead(kEncoderButtonPins[i]));
  }
  for (uint8_t i = 0; i < kFunctionCount; ++i) {
    pinMode(kFunctionPins[i], INPUT_PULLUP);
    initializeDebouncer(functionButtons[i], digitalRead(kFunctionPins[i]));
  }
  for (uint8_t i = 0; i < kMatrixSize; ++i) {
    pinMode(kColumnPins[i], INPUT_PULLUP);
    pinMode(kRowPins[i], INPUT);
  }
  for (DebouncedInput &button : matrixButtons) initializeDebouncer(button, HIGH);

  for (uint8_t i = 0; i < kAnalogCount; ++i) analogValues[i] = analogRead(kAnalogPins[i]);

  lcd.begin(20, 2);
  lcd.print("MIDIcon I/O test");
  lcd.setCursor(0, 1);
  lcd.print("Ready");

  leds.begin();
  leds.setBrightness(32);
  leds.clear();
  leds.show();

  Serial.println(F("MIDIcon I/O test ready"));
  Serial.println(F("Buttons/matrix are debounced for 20 ms; MIDI IN echoes to MIDI OUT."));
}

void loop() {
  const uint32_t now = millis();
  scanEncoders();
  scanDirectButtons(now);
  scanMatrix(now);
  scanAnalog(now);
  updateLeds(now);
  serviceMidi();
}
