#include "Controller.h"
#include "Persistence.h"
#include <string.h>
namespace midicon {
constexpr uint8_t EA[4] = {5, 11, 17, 25}, EB[4] = {7, 13, 21, 27},
                  EP[4] = {3, 9, 15, 23}, FP[4] = {33, 35, 37, 39};
constexpr uint8_t COL[4] = {24, 26, 28, 30}, ROW[4] = {41, 43, 45, 47},
                  AP[8] = {A0, A2, A4, A6, A8, A10, A12, A14};
// Joystick 2 (A12/A14) is intentionally disabled until it is connected.
constexpr uint8_t ACTIVE_ANALOG_COUNT = 6;
// Ben Buxton-style full-step Gray-code decoder. State changes caused by contact
// bounce cannot produce an event unless the encoder completes a valid cycle.
constexpr uint8_t DIR_CW = 0x10, DIR_CCW = 0x20;
constexpr uint8_t ENCODER_TABLE[7][4] = {
    {0, 2, 4, 0}, {3, 0, 1, DIR_CW},  {3, 2, 0, 0}, {3, 2, 1, 0},
    {6, 0, 4, 0}, {6, 5, 0, DIR_CCW}, {6, 5, 4, 0},
};
Controller::Controller()
    : lcd_(36, 38, 40, 42, 44, 46), leds_(4, 32, NEO_RGB + NEO_KHZ800) {}
bool Controller::Debounce::update(bool v, uint32_t n) {
    if (v != sample) {
        sample = v;
        at = n;
    }
    if (stable != sample && n - at >= 20) {
        stable = sample;
        return true;
    }
    return false;
}
void Controller::begin() {
    Serial.begin(115200);
    Serial1.begin(31250);
    if (!Persistence::load(config_))
        makeDefaults(config_);
    for (uint8_t i = 0; i < 4; i++) {
        pinMode(EA[i], INPUT_PULLUP);
        pinMode(EB[i], INPUT_PULLUP);
        pinMode(EP[i], INPUT_PULLUP);
        pinMode(FP[i], INPUT_PULLUP);
        encoderDecoderState_[i] = 0;
    }
    for (uint8_t i = 0; i < 4; i++) {
        pinMode(COL[i], INPUT_PULLUP);
        pinMode(ROW[i], INPUT);
    }
    for (uint8_t i = 0; i < ACTIVE_ANALOG_COUNT; i++)
        raw_[i] = filtered_[i] = analogRead(AP[i]);
    for (uint8_t p = 0; p < 4; ++p) {
        for (uint8_t e = 0; e < 4; ++e)
            logical_[p][e] = config_.mappings[p][16 + e].initial;
        for (uint8_t a = 0; a < 8; ++a)
            logical_[p][4 + a] = config_.mappings[p][24 + a].initial;
    }
    lcd_.begin(20, 2);
    leds_.begin();
    leds_.setBrightness(32);
    strcpy(lower_, "READY");
    render(millis(), true);
}
void Controller::tick() {
    uint32_t n = millis();
    midiIn(n);
    protocol();
    scan(n);
    analog(n);
    for (uint8_t i = 0; i < 16; i++)
        if (triggerAt_[i] && int32_t(n - triggerAt_[i]) >= 0) {
            send3(0x80 | ((triggerOff_[i] >> 8) & 15), triggerOff_[i] & 127, 0);
            triggerAt_[i] = 0;
        }
    if (mode_ == Mode::Message && int32_t(n - messageUntil_) >= 0)
        mode_ = Mode::System;
    render(n);
}
void Controller::scan(uint32_t n) {
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t pins = (digitalRead(EA[i]) << 1) | digitalRead(EB[i]);
        uint8_t result = ENCODER_TABLE[encoderDecoderState_[i] & 0x0F][pins];
        encoderDecoderState_[i] = result & 0x0F;
        if (result & DIR_CW)
            encoderEvent(i, 1);
        else if (result & DIR_CCW)
            encoderEvent(i, -1);
        if (encPb_[i].update(digitalRead(EP[i]), n))
            encoderButton(i, !encPb_[i].stable);
        if (functions_[i].update(digitalRead(FP[i]), n))
            functionEvent(i, !functions_[i].stable);
    }
    for (uint8_t r = 0; r < 4; r++) {
        pinMode(ROW[r], OUTPUT);
        digitalWrite(ROW[r], LOW);
        delayMicroseconds(3);
        for (uint8_t c = 0; c < 4; c++) {
            uint8_t i = r * 4 + c;
            if (matrix_[i].update(digitalRead(COL[c]), n))
                matrixEvent(i, !matrix_[i].stable);
        }
        pinMode(ROW[r], INPUT);
    }
}
void Controller::functionEvent(uint8_t i, bool down) {
    if (down) {
        if (functionOwner_ == 255) {
            functionOwner_ = i;
            functionConsumed_ = false;
            releaseNotes();
        }
        return;
    }
    if (functionOwner_ != i)
        return;
    if (mode_ == Mode::Performance && !functionConsumed_)
        changePage(i);
    else if (mode_ == Mode::System) {
        if (i == 0)
            systemAccept();
        else if (i == 3) {
            mode_ = Mode::Performance;
            releaseNotes();
        }
    } else if (mode_ == Mode::LoadConfirm) {
        if (i == 0) {
            Configuration c;
            if (Persistence::load(c)) {
                releaseNotes();
                config_ = c;
                dirty_ = false;
                show("LOADED");
            } else
                show("EEPROM INVALID");
        } else if (i == 1 || i == 3)
            mode_ = Mode::System;
    } else if (mode_ == Mode::AssignSelect) {
        if (i == 0 && selected_ != 255) {
            edit_ = mapping(selected_);
            mode_ = Mode::AssignEdit;
        } else if (i == 3)
            mode_ = Mode::Performance;
    } else if (mode_ == Mode::AssignEdit) {
        if (i == 0) {
            mapping(selected_) = edit_;
            dirty_ = true;
            mode_ = Mode::AssignSelect;
        } else if (i == 1) {
            mode_ = Mode::AssignSelect;
        } else if (i == 3)
            mode_ = Mode::Performance;
    } else if (mode_ == Mode::Calibration) {
        if (i == 0) {
            if (calibrationIndex_ >= 4)
                config_.analog[calibrationIndex_].center =
                    raw_[calibrationIndex_];
            if (++calibrationIndex_ >= ACTIVE_ANALOG_COUNT) {
                if (validate(config_)) {
                    dirty_ = true;
                    mode_ = Mode::System;
                } else {
                    memcpy(config_.analog, calibrationBackup_,
                           sizeof(calibrationBackup_));
                    show("CALIBRATION ERROR");
                }
            }
        } else if (i == 1 || i == 3) {
            memcpy(config_.analog, calibrationBackup_,
                   sizeof(calibrationBackup_));
            mode_ = Mode::System;
        }
    }
    if (mode_ == Mode::Performance) {
        for (uint8_t f = 0; f < 4; ++f) {
            uint8_t physical = scaled(f, filtered_[f]);
            pickupLast_[page_][f] = physical;
            pickup_[page_][f] =
                abs(int(physical) - int(logical_[page_][4 + f])) >
                config_.pickupTolerance;
        }
    }
    functionOwner_ = 255;
    functionConsumed_ = false;
    render(millis(), true);
}
void Controller::matrixEvent(uint8_t i, bool down) {
    if (functionOwner_ != 255) {
        if (down) {
            functionConsumed_ = true;
            shiftOnly_[i] = true;
            if (functionOwner_ == 3 && i == 0) {
                releaseNotes();
                mode_ = Mode::System;
                systemItem_ = 0;
            } else if (functionOwner_ == 3 && i == 15) {
                releaseNotes();
                mode_ = Mode::AssignSelect;
                selected_ = 255;
            }
            render(millis(), true);
        } else
            shiftOnly_[i] = false;
        return;
    }
    if (mode_ == Mode::AssignSelect && down) {
        selected_ = i;
        render(millis(), true);
        return;
    }
    if (mode_ == Mode::Performance)
        dispatchButton(i, down);
}
void Controller::encoderButton(uint8_t i, bool down) {
    if (mode_ == Mode::AssignSelect && down) {
        selected_ = 20 + i;
        render(millis(), true);
        return;
    }
    if (mode_ == Mode::AssignEdit && down) {
        editGroup_ = (editGroup_ + 1) % 2;
        render(millis(), true);
        return;
    }
    if (mode_ == Mode::Performance && functionOwner_ == 255)
        dispatchButton(20 + i, down);
}
void Controller::encoderEvent(uint8_t i, int8_t d) {
    if (mode_ == Mode::System && i == 0) {
        systemItem_ = (systemItem_ + d + 4) % 4;
        render(millis(), true);
    } else if (mode_ == Mode::AssignSelect) {
        selected_ = 16 + i;
        render(millis(), true);
    } else if (mode_ == Mode::AssignEdit)
        assignmentAdjust(i, d);
    else if (mode_ == Mode::Calibration && i == 0) {
        calibrationIndex_ =
            (calibrationIndex_ + d + ACTIVE_ANALOG_COUNT) % ACTIVE_ANALOG_COUNT;
        render(millis(), true);
    } else if (mode_ == Mode::Performance && functionOwner_ == 255)
        dispatchEncoder(i, d);
}
void Controller::dispatchButton(uint8_t idx, bool down) {
    Mapping &m = mapping(idx);
    if (m.action == 0)
        return;
    bool fire = down;
    if (m.behavior == 2 && down) {
        toggle_[page_][idx] = !toggle_[page_][idx];
        fire = toggle_[page_][idx];
    }
    if (m.behavior == 2 && !down)
        return;
    if (m.behavior == 3 && !down)
        return;
    uint8_t st = (m.channel - 1) & 15;
    if (m.action == 1) {
        send3((fire ? 0x90 : 0x80) | st, m.number,
              fire ? m.maximum : m.minimum);
        if (idx < 16)
            noteHeld_[idx] = fire;
        if (m.behavior == 3) {
            triggerOff_[idx] = (st << 8) | m.number;
            triggerAt_[idx] =
                millis() + (m.durationMs < 10 ? 10 : m.durationMs);
        }
    } else if (m.action == 2)
        send3(0xB0 | st, m.number, fire ? m.maximum : m.minimum);
    else if (m.action == 3 && down)
        send2(0xC0 | st, m.number);
    snprintf(lower_, 21, "BTN%02u %s%03u CH%u", idx + 1,
             m.action == 1 ? "N" : "CC", m.number, m.channel);
}
void Controller::dispatchEncoder(uint8_t i, int8_t d) {
    Mapping &m = mapping(16 + i);
    uint16_t &v = logical_[page_][i];
    if (m.behavior == 5) {
        send3(0xB0 | (m.channel - 1), m.number, d > 0 ? 1 : 127);
    } else {
        int s = m.step ? m.step : 1;
        v = constrain(int(v) + d * s, m.minimum, m.maximum);
        send3(0xB0 | (m.channel - 1), m.number, v);
    }
    snprintf(lower_, 21, "ENC%02u CC%03u CH%u %u", i + 1, m.number, m.channel,
             v);
}
uint8_t Controller::scaled(uint8_t i, uint16_t v) const {
    auto &a = config_.analog[i];
    v = constrain(v, a.minimum, a.maximum);
    long x;
    if (i >= 4) {
        if (abs(int(v) - int(a.center)) <= 8)
            return 64;
        x = v < a.center ? map(v, a.minimum, a.center, 0, 64)
                         : map(v, a.center, a.maximum, 64, 127);
    } else {
        x = map(v, a.minimum, a.maximum, 0, 127);
    }
    return constrain(x, 0, 127);
}
void Controller::analog(uint32_t n) {
    static uint32_t at = 0;
    if (n - at < 5)
        return;
    at = n;
    for (uint8_t i = 0; i < ACTIVE_ANALOG_COUNT; i++) {
        uint16_t v = analogRead(AP[i]);
        filtered_[i] = (filtered_[i] * 3 + v) / 4;
        int delta = abs(int(filtered_[i]) - int(raw_[i]));
        if (delta < 4)
            continue;
        raw_[i] = filtered_[i];
        if (mode_ == Mode::Calibration && i == calibrationIndex_) {
            Calibration &cal = config_.analog[i];
            if (raw_[i] < cal.minimum)
                cal.minimum = raw_[i];
            if (raw_[i] > cal.maximum)
                cal.maximum = raw_[i];
            render(n, true);
            continue;
        }
        if (mode_ == Mode::AssignSelect && delta >= 16) {
            selected_ = 24 + i;
            render(n, true);
        }
        if (mode_ != Mode::Performance || functionOwner_ != 255)
            continue;
        dispatchAnalog(i, scaled(i, filtered_[i]));
    }
}
void Controller::dispatchAnalog(uint8_t i, uint8_t v) {
    Mapping &m = mapping(24 + i);
    if (m.action == 0)
        return;
    if (i < 4 && pickup_[page_][i]) {
        int target = logical_[page_][4 + i];
        uint8_t previous = pickupLast_[page_][i];
        pickupLast_[page_][i] = v;
        bool crossed = (previous <= target && v >= target) ||
                       (previous >= target && v <= target);
        if (!crossed && abs(int(v) - target) > config_.pickupTolerance) {
            snprintf(lower_, 21, "FDR%02u PICKUP %03u", i + 1, target);
            return;
        }
        pickup_[page_][i] = false;
    }
    if (v == logical_[page_][4 + i])
        return;
    logical_[page_][4 + i] = v;
    uint8_t out = (m.flags & 1) ? 127 - v : v;
    out = map(out, 0, 127, m.minimum, m.maximum);
    if (m.action == 2)
        send3(0xB0 | (m.channel - 1), m.number, out);
    else if (m.action == 4) {
        uint16_t pb = out * 129;
        send3(0xE0 | (m.channel - 1), pb & 127, (pb >> 7) & 127);
    }
    snprintf(lower_, 21, "%s%02u CC%03u CH%u %u", i < 4 ? "FDR" : "JOY",
             i < 4 ? i + 1 : i - 3, m.number, m.channel, out);
}
void Controller::changePage(uint8_t p) {
    if (p == page_)
        return;
    releaseNotes();
    page_ = p;
    for (uint8_t i = 0; i < 4; i++) {
        pickup_[p][i] = true;
        pickupLast_[p][i] = scaled(i, filtered_[i]);
    }
    strcpy(lower_, "PAGE CHANGED");
}
void Controller::releaseNotes() {
    for (uint8_t i = 0; i < 16; i++)
        if (noteHeld_[i]) {
            Mapping &m = mapping(i);
            send3(0x80 | (m.channel - 1), m.number, 0);
            noteHeld_[i] = false;
        }
}
void Controller::sendByte(uint8_t b) {
    if (!sysex_)
        Serial1.write(b);
    else {
        uint8_t next = (midiHead_ + 1) & 63;
        if (next != midiTail_) {
            midiQueue_[midiHead_] = b;
            midiHead_ = next;
        }
    }
}
void Controller::flushMidiQueue() {
    while (midiTail_ != midiHead_) {
        Serial1.write(midiQueue_[midiTail_]);
        midiTail_ = (midiTail_ + 1) & 63;
    }
}
void Controller::send3(uint8_t a, uint8_t b, uint8_t c) {
    sendByte(a);
    sendByte(b & 127);
    sendByte(c & 127);
}
void Controller::send2(uint8_t a, uint8_t b) {
    sendByte(a);
    sendByte(b & 127);
}
void Controller::midiIn(uint32_t n) {
    while (Serial1.available()) {
        uint8_t b = Serial1.read();
        Serial1.write(b);
        if (b == 0xF0)
            sysex_ = true;
        else if (b == 0xF7) {
            sysex_ = false;
            flushMidiQueue();
        }
        if (b == 0xF8) {
            if (lastClock_) {
                uint32_t d = n - lastClock_;
                if (d) {
                    float x = 2500.0f / d;
                    bpm_ = bpm_ ? bpm_ * .85f + x * .15f : x;
                }
            }
            lastClock_ = n;
            clockAt_ = n;
        } else if (b == 0xFA || b == 0xFB)
            transport_ = true;
        else if (b == 0xFC)
            transport_ = false;
    }
    if (clockAt_ && n - clockAt_ > 2000)
        bpm_ = 0;
}
void Controller::systemAccept() {
    if (systemItem_ == 0) {
        Persistence::save(config_);
        dirty_ = false;
        show("EEPROM SAVED");
    } else if (systemItem_ == 1) {
        if (dirty_)
            mode_ = Mode::LoadConfirm;
        else {
            Configuration c;
            if (Persistence::load(c)) {
                config_ = c;
                show("EEPROM LOADED");
            } else
                show("EEPROM INVALID");
        }
    } else if (systemItem_ == 2) {
        memcpy(calibrationBackup_, config_.analog, sizeof(calibrationBackup_));
        for (uint8_t i = 0; i < ACTIVE_ANALOG_COUNT; ++i) {
            config_.analog[i].minimum = 1023;
            config_.analog[i].maximum = 0;
        }
        calibrationIndex_ = 0;
        mode_ = Mode::Calibration;
    } else
        show("UNASSIGNED");
}
void Controller::show(const char *s) {
    strncpy(message_, s, 20);
    message_[20] = 0;
    mode_ = Mode::Message;
    messageUntil_ = millis() + 1200;
    render(millis(), true);
}
void Controller::assignmentAdjust(uint8_t e, int8_t d) {
    if (e == 0)
        edit_.action = (edit_.action + d + 5) % 5;
    else if (e == 1)
        edit_.behavior = constrain(int(edit_.behavior) + d, 1, 6);
    else if (e == 2)
        edit_.number = (edit_.number + d + 128) % 128;
    else
        edit_.channel = (edit_.channel - 1 + d + 16) % 16 + 1;
    render(millis(), true);
}
void Controller::ledRender() {
    uint32_t off = leds_.Color(0, 0, 0), green = leds_.Color(0, 80, 0),
             amber = leds_.Color(60, 20, 0), red = leds_.Color(80, 0, 0);
    auto physicalLed = [](uint8_t logical) { return uint8_t(3 - logical); };
    for (uint8_t i = 0; i < 4; i++)
        leds_.setPixelColor(i, off);
    if (mode_ == Mode::Performance)
        leds_.setPixelColor(physicalLed(page_), leds_.Color(0, 0, 80));
    else {
        leds_.setPixelColor(physicalLed(0), green);
        leds_.setPixelColor(physicalLed(3), amber);
        if (mode_ == Mode::AssignEdit || mode_ == Mode::LoadConfirm)
            leds_.setPixelColor(physicalLed(1), red);
    }
    leds_.show();
}
void Controller::render(uint32_t n, bool force) {
    if (!force && n - lastUi_ < 100)
        return;
    lastUi_ = n;
    char a[21], b[21];
    memset(a, ' ', 20);
    memset(b, ' ', 20);
    a[20] = b[20] = 0;
    if (mode_ == Mode::Performance) {
        snprintf(a, 21, "P%u %-8.8s %s", page_ + 1, config_.pageNames[page_],
                 bpm_ ? "BPM" : "---");
        if (bpm_) {
            uint16_t x = bpm_ * 10;
            snprintf(a + 13, 8, "%3u.%u", x / 10, x % 10);
        }
        strncpy(b, lower_, 20);
    } else if (mode_ == Mode::System) {
        const char *opts[4] = {"SAVE EEP", "LOAD EEP", "CALIBR8", "????????"};
        snprintf(a, 21, "%c%-8s %c%-8s", systemItem_ == 0 ? '>' : ' ', opts[0],
                 systemItem_ == 1 ? '>' : ' ', opts[1]);
        snprintf(b, 21, "%c%-8s %c%-8s", systemItem_ == 2 ? '>' : ' ', opts[2],
                 systemItem_ == 3 ? '>' : ' ', opts[3]);
    } else if (mode_ == Mode::LoadConfirm) {
        strcpy(a, "DISCARD CHANGES?");
        strcpy(b, "F1 YES  F2/F4 NO");
    } else if (mode_ == Mode::AssignSelect) {
        strcpy(a, "ASSIGN: TOUCH CTRL");
        if (selected_ == 255)
            strcpy(b, "F4 EXIT");
        else
            snprintf(b, 21, "CTRL %02u F1 EDIT", selected_ + 1);
    } else if (mode_ == Mode::AssignEdit) {
        snprintf(a, 21, "A%u B%u N%03u C%02u", edit_.action, edit_.behavior,
                 edit_.number, edit_.channel);
        strcpy(b, "F1 OK F2 NO F4 X");
    } else if (mode_ == Mode::Calibration) {
        snprintf(a, 21, "CAL %u RAW %4u", calibrationIndex_ + 1,
                 raw_[calibrationIndex_]);
        snprintf(b, 21, "F1 NEXT F2 CANCEL");
    } else {
        strncpy(a, message_, 20);
        strcpy(b, "PLEASE WAIT");
    }
    for (uint8_t i = strlen(a); i < 20; ++i)
        a[i] = ' ';
    for (uint8_t i = strlen(b); i < 20; ++i)
        b[i] = ' ';
    a[20] = b[20] = 0;
    lcd_.setCursor(0, 0);
    lcd_.print(a);
    lcd_.setCursor(0, 1);
    lcd_.print(b);
    ledRender();
}
// USB frame: A5, command, uint16 length, payload, CRC16(command..payload).
void Controller::protocol() {
    static uint8_t buf[sizeof(Configuration) + 6];
    static uint16_t used = 0, need = 0;
    while (Serial.available()) {
        uint8_t x = Serial.read();
        if (!used && x != 0xA5)
            continue;
        buf[used++] = x;
        if (used == 4)
            need = 4 + buf[2] + (uint16_t(buf[3]) << 8) + 2;
        if (need && used >= need) {
            uint16_t got = buf[need - 2] | uint16_t(buf[need - 1]) << 8,
                     calc = crc16(buf + 1, need - 3);
            uint8_t cmd = buf[1], status = got == calc ? 0 : 1;
            if (!status && cmd == 1) {
            } else if (!status && cmd == 2) {
                Serial.write((uint8_t *)&config_, sizeof(config_));
            } else if (!status && cmd == 3 && need - 6 == sizeof(config_)) {
                Configuration t;
                memcpy(&t, buf + 4, sizeof(t));
                if (validate(t)) {
                    releaseNotes();
                    config_ = t;
                    dirty_ = true;
                } else
                    status = 2;
            } else if (!status && cmd == 4) {
                status = Persistence::save(config_) ? 0 : 3;
                if (!status)
                    dirty_ = false;
            } else if (!status && cmd == 5) {
                Configuration t;
                if (Persistence::load(t)) {
                    releaseNotes();
                    config_ = t;
                    dirty_ = false;
                } else
                    status = 3;
            } else if (!status && cmd == 6)
                status = dirty_;
            else if (cmd > 6)
                status = 4;
            Serial.write(0x5A);
            Serial.write(cmd);
            Serial.write(status);
            used = need = 0;
        }
        if (used >= sizeof(buf))
            used = need = 0;
    }
}
} // namespace midicon
