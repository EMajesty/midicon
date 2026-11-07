# MIDI Controller — MCU Pinout and Wiring Reference

MCU: Arduino Mega 2560 Pro  
Logic voltage: 5 V

## 1. MCU Pin Assignments

| Arduino Pin | Net | Function |
|---|---|---|
| D3 | E1_PB | Encoder 1 push button |
| D5 | E1_A | Encoder 1 channel A |
| D7 | E1_B | Encoder 1 channel B |
| D9 | E2_PB | Encoder 2 push button |
| D11 | E2_A | Encoder 2 channel A |
| D14 | E2_B | Encoder 2 channel B |
| D15 | E3_PB | Encoder 3 push button |
| D17 | E3_A | Encoder 3 channel A |
| D18 | MIDI_OUT | MIDI output (TX1) |
| D19 | MIDI_IN | MIDI input (RX1) |
| D21 | E3_B | Encoder 3 channel B |
| D23 | E4_PB | Encoder 4 push button |
| D24 | C1 | Matrix column 1 |
| D25 | E4_A | Encoder 4 channel A |
| D26 | C2 | Matrix column 2 |
| D27 | E4_B | Encoder 4 channel B |
| D28 | C3 | Matrix column 3 |
| D30 | C4 | Matrix column 4 |
| D32 | LEDS | WS2812B data |
| D33 | F1 | Function button 1 |
| D35 | F2 | Function button 2 |
| D36 | LCD_RS | LCD register select |
| D37 | F3 | Function button 3 |
| D38 | LCD_E | LCD enable |
| D39 | F4 | Function button 4 |
| D40 | LCD_DB4 | LCD data bit 4 |
| D41 | R1 | Matrix row 1 |
| D42 | LCD_DB5 | LCD data bit 5 |
| D43 | R2 | Matrix row 2 |
| D44 | LCD_DB6 | LCD data bit 6 |
| D45 | R3 | Matrix row 3 |
| D46 | LCD_DB7 | LCD data bit 7 |
| D47 | R4 | Matrix row 4 |
| A0 | ADC_F1 | Fader 1 |
| A2 | ADC_F2 | Fader 2 |
| A4 | ADC_F3 | Fader 3 |
| A6 | ADC_F4 | Fader 4 |
| A8 | ADC_J1X | Joystick 1 X |
| A10 | ADC_J1Y | Joystick 1 Y |
| A12 | ADC_J2X | Joystick 2 X |
| A14 | ADC_J2Y | Joystick 2 Y |

D0/D1 are reserved for USB serial debugging.

## 2. Rotary Encoders

Four rotary encoders with integrated push buttons.

| Contact | Connection |
|---|---|
| A | E*_A MCU input |
| B | E*_B MCU input |
| Common | GND |
| Push button terminal 1 | E*_PB MCU input |
| Push button terminal 2 | GND |

- All 12 inputs use `INPUT_PULLUP`.
- Rotary contacts pull their respective inputs LOW when closed.
- Push buttons are active LOW.
- Quadrature decoding and debouncing are handled in firmware.

## 3. Function Buttons

Four independent momentary buttons.

| Contact | Connection |
|---|---|
| Terminal 1 | F1–F4 MCU input |
| Terminal 2 | GND |

- Configure as `INPUT_PULLUP`.
- Pressed = LOW.

## 4. Button Matrix

4×4 matrix with one diode per button.

| Row | C1 (D24) | C2 (D26) | C3 (D28) | C4 (D30) |
|---|---|---|---|---|
| R1 (D41) | SW1 | SW2 | SW3 | SW4 |
| R2 (D43) | SW5 | SW6 | SW7 | SW8 |
| R3 (D45) | SW9 | SW10 | SW11 | SW12 |
| R4 (D47) | SW13 | SW14 | SW15 | SW16 |

### Wiring

Each button is wired:

`Column → Switch → Diode anode → Diode cathode → Row`

- Diode cathode (striped end) faces the row.
- One diode per switch prevents ghosting.
- Columns are configured as `INPUT_PULLUP`.
- Scan by driving one row LOW at a time.
- Inactive rows are high-impedance (`INPUT`).
- A pressed button on the active row reads LOW at its column.

## 5. Analog Controls

Four stereo faders (one track used per fader) and two dual-axis joysticks.

| Control | ADC Pin |
|---|---|
| Fader 1 | A0 |
| Fader 2 | A2 |
| Fader 3 | A4 |
| Fader 4 | A6 |
| Joystick 1 X | A8 |
| Joystick 1 Y | A10 |
| Joystick 2 X | A12 |
| Joystick 2 Y | A14 |

### Wiring

Each potentiometer:

| Terminal | Connection |
|---|---|
| End 1 | +5 V |
| Wiper | ADC input |
| End 3 | GND |

- ADC range: 0–1023.
- Reversing the outer terminals reverses the control direction.
- Unused tracks of stereo faders are left unconnected.
- Joystick axes each use one potentiometer.

## 6. WS2812B LEDs

Four addressable RGB LEDs connected in series.

| Signal | Connection |
|---|---|
| First LED DIN | D32 (LEDS) |
| Each LED DOUT | Next LED DIN |
| All VDD | +5 V |
| All VSS | GND |
| Final LED DOUT | Unconnected |

All LEDs are controlled through one MCU pin.

## 7. LCD

20×2 character LCD, 4-bit parallel interface.

Module markings: Winstar WDH0048-TMI-ET#00 / Forewin 2002AE-1.

The following assumes a standard HD44780-compatible 16-pin interface.

| LCD Pin | Signal | Connection |
|---|---|---|
| 1 | VSS | GND |
| 2 | VDD | +5 V |
| 3 | VO | Contrast potentiometer wiper |
| 4 | RS | D36 |
| 5 | R/W | GND |
| 6 | E | D38 |
| 7–10 | DB0–DB3 | Unconnected |
| 11 | DB4 | D40 |
| 12 | DB5 | D42 |
| 13 | DB6 | D44 |
| 14 | DB7 | D46 |
| 15 | LED+ | +5 V (verify current limiting) |
| 16 | LED− | GND |

### Contrast potentiometer (RV9)

| Terminal | Connection |
|---|---|
| End 1 | +5 V |
| Wiper | LCD VO |
| End 3 | GND |

Firmware initialization:

`LiquidCrystal lcd(36, 38, 40, 42, 44, 46);`

## 8. MIDI Input

5-pin DIN MIDI input using a 6N137 optocoupler.

## 9. MIDI Output

5-pin DIN MIDI output driven by hardware UART1.

Firmware:

`Serial1.begin(31250);`

## 10. Power Supply

9 V DC input regulated to 5 V using an L7805.

| Component | Connection |
|---|---|
| Barrel jack positive | +9 V |
| Barrel jack negative | GND |
| L7805 IN | +9 V |
| L7805 GND | GND |
| L7805 OUT | +5 V |
| C2 | Input decoupling |
| C1 | Output decoupling |

The +5 V rail supplies the MCU, controls, LCD, MIDI circuitry and LEDs.

## Firmware and configurator

The v1 firmware is a standalone, four-page configurable MIDI controller. Build it with `pio run -d firmware`; see [firmware/README.md](firmware/README.md) for operation, EEPROM layout, MIDI conventions, and the USB protocol.

The native Python configurator provides JSON validation/import/export, device upload/download, explicit EEPROM save/reload, and a PySide6 GUI:

```sh
python -m pip install -r requirements.txt
python -m configurator validate config/default.json
python -m configurator gui
```

See [configurator/README.md](configurator/README.md) for all commands and [docs/hardware-test-checklist.md](docs/hardware-test-checklist.md) for physical verification.
