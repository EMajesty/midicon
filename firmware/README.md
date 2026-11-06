# MIDIcon v1 firmware

The firmware targets the Mega 2560 Pro wiring in the root README. It is a standalone four-page MIDI controller; USB serial is used only by the configurator, while UART1 on D18/D19 carries DIN MIDI at 31250 baud.

## Build and upload

```sh
pio run -d firmware
pio run -d firmware -t upload
```

The four pages each contain matrix buttons 1–16, encoders 1–4, encoder switches 1–4, faders 1–4, and joystick axes J1X/J1Y/J2X/J2Y. Defaults use notes 36–51 for pads and CC messages for continuous controls. F1–F4 select pages when tapped. Holding a function button suppresses mapped output; only F4+pad 1 (SYSTEM) and F4+pad 16 (ASSIGN) have commands.

SYSTEM provides SAVE EEP, LOAD EEP, CALIBR8, and an inert placeholder. EEPROM uses two 2048-byte slots; a header contains a monotonic sequence, CRC-16, and a commit byte written last. The 1634-byte packed configuration therefore fits twice. Writes occur only on explicit save.

APA106 LEDs use the WS281x 800 kHz protocol through Adafruit NeoPixel. The configured order is `NEO_RGB`; verify red/green on the physical batch and change the constructor flag if necessary. A four-pixel update masks interrupts briefly (roughly 120 microseconds); UART buffering should tolerate this, but dense-clock bench testing is required.

## Relative encoder convention

Relative CC sends `1` for clockwise and `127` for counterclockwise (binary offset/sign convention). Absolute and stepped modes retain a separate logical value per page.

## USB protocol

Requests are `A5 command uint16_le_length payload crc16_le`, where CRC-16/Modbus covers command, length, and payload. Replies end in `5A command status`. Commands: identify (1), download working configuration (2), atomic apply to RAM (3), explicit EEPROM save (4), EEPROM reload (5), and dirty status (6). A complete configuration is validated before activation.

## Calibration

Enter SYSTEM with F4+pad 1 and choose CALIBR8. The display exposes raw ADC readings for bench calibration. Calibration remains a RAM change until SAVE EEP. Never save zero-width ranges; invalid records are rejected by configuration validation.
