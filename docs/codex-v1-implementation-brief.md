# MIDIcon v1 implementation brief for Codex

## Goal
Implement a standalone, configurable performance MIDI controller based on the existing working I/O-test firmware. Work in incremental testable milestones, keep hardware configuration authoritative, and do not require a laptop during performance. Do not overwrite the main branch.

## Existing repo findings (2026-11-06)
- Arduino Mega 2560 Pro, PlatformIO project in `firmware/`, existing `firmware/src/main.cpp` scans buttons/encoders, LCD, four NeoPixel-compatible LEDs and echoes MIDI input bytes to output on UART1.
- D18 = TX1 MIDI OUT; D19 = RX1 MIDI IN; use `Serial1.begin(31250)`. D0/D1 serial via CH340 for configurator/debug.
- Root README provides pinout, schematic `hardware/midicon.kicad_sch` and PCB. Read actual repo hardware and current code before changing.
- **Existing firmware scans only six analog channels** A0 A2 A4 A6 A8 A10; root README documents **eight** channels including A12 J2X and A14 J2Y. Include all eight.
- D32 drives four chained RGB LEDs. User calls installed LEDs APA106; repo README / schematic identify WS2812B. Validate signal protocol/color order for actual installed parts; avoid changing hardware assumptions silently.
- LCD pins: RS36 E38 DB4 40 DB5 42 DB6 44 DB7 46, 20×2 `LiquidCrystal`.
- 4×4 buttons matrix columns D24/26/28/30 and rows D41/43/45/47. Diode cathodes toward rows; scanned one row LOW, columns pulled up.
- Encoders A D5/11/17/25 B D7/13/21/27, switch D3/9/15/23; F1–F4 D33/35/37/39. All direct buttons active LOW.
- Project uses Nix `flake.nix` and PlatformIO; preserve these setups.

## User-locked interaction decisions
- Four **fully configurable performance pages**. Each page maps 16 pads + 4 encoder rotations + 4 encoder switches + 4 faders + 4 joystick axes = 32 slots, 128 total.
- Defaults: matrix momentary MIDI Note; encoders absolute CC; joysticks centered; faders soft takeover enabled. Single action per slot for v1, but use generic action dispatch and compact reference structure so macros can be added later without rewiring input logic.
- F1–F4 are page buttons, **not user-MIDI-assignable**. Tap and release without chord to change page. Holding one and then pressing a matrix button executes a **global firmware-defined** F-key × pad command. The mapping of each of 64 possible combos is fixed in firmware, not configurable on device or in Python. Effects of commands may reference the current page. For v1, **only F4 + pad16 is bound**, to enter assignment mode on the current page; all other shifted pad actions are unassigned. Any pressed matrix button while held consumes modifier, even if no action assigned; releasing modifier then must not change page. First pressed function wins if simultaneous; define safe ignore behavior for others.
- No shifted encoder/fader/joystick functions in v1. Suppress MIDI from controls while F held; movement does not consume modifier except matrix presses. Analog sampling continues; track physical faders and prevent jumps after hold.
- **Assignment workflow:** F4+pad16 enters assignment selection for active page. In selection, latest *meaningfully* changed control (pad press, encoder rotation, encoder push, sufficiently moved fader or joystick axis) becomes selected. F buttons not selectable. Filter analog noise, use movement threshold, hysteresis, and do not allow joystick return-to-center or analog jitter to continuously steal selection. F1 advances to editing selected control. In edit, four encoders navigate hierarchical action category/behavior/options/values (implement compact context-sensitive layout suitable for 20×2; encoder switches can switch parameter groups). F1 APPLIES changed assignment **to RAM only** then returns to selection. F2 DISCARDs in-progress changes and returns to selection, leaving prior RAM-applied changes intact. F4 exits assignment mode, discarding only in-progress editor changes, retaining previous RAM edits. No EEPROM writes on F1 apply, F2 discard, or F4 exit. Normal performance output from controls must be suppressed throughout assignment mode, but incoming MIDI passthrough must continue. Consider held notes on entering/exiting mode and stable analog state.
- Implement separate **explicit whole-configuration EEPROM Save** and reload/discard functionality, reachable through Python configurator initially if on-device shortcut not yet designed; do not invent more shifted pad shortcuts. Explain how the user can persist on device later, and reserve unassigned shortcut for future design rather than automatic writes. Detect unsaved/dirty state. Protect from partial/invalid configurations, use checksum and schema version, assess whether dual slots fit 4KB EEPROM. Reset invalid config to safe defaults, do not crash. Avoid EEPROM writes at boot or on ordinary page change.
- Analog faders: physical and logical MIDI values independent, soft takeover/pickup on page change and temporary suppression; do not transmit until crossing last logical value (with reasonable tolerance). Pick-up only if necessary; unsuppressed motion resumes normally. Each page retains independent logical states. Joystick centered behavior should return to neutral correctly without fader-style pickup; use calibration/dead zone.
- Buttons momentary/toggle/one-shot; note on/off lifecycle safe even across page changes/mode changes. Encoders absolute/relative/stepped. Basic MIDI Note, CC, Program Change and Pitch Bend where applicable; 7-bit channel values and MIDI channel 1–16. Rate-limit/dedupe analog MIDI; fixed-sized buffers / static allocation on 8KB SRAM.
- MIDI IN -> OUT **passthrough and merger** v1, including MIDI clock, realtime Start/Continue/Stop, system common, SysEx, and channel traffic. Preserve bytes/ordering/message validity; realtime can be interleaved, prioritize clock enough not to drop events; merging local MIDI must not corrupt SysEx/running status. UART1 31250. No echo feedback loops, no duplicate generated clock when upstream clock exists. USB CH340 console/config serial is separate. Avoid slow blocking debug logs or LED/LCD operations during MIDI traffic. A test that exercises densely interleaved clock + notes + SysEx + local CC is mandatory.
- Show incoming BPM derived from 24 PPQN clock with smoothed value and timeout. Upper LCD row: active page name/number + compact clock/BPM and eventually transport info. Lower row: latest touched control and its outgoing MIDI message/value (future macro name). Limit LCD writes / update cadence.
- Four addressable LEDs beneath F1–F4 show active page in performance (active illuminated, others dim), and context actions in assignment mode: F1 green for edit/apply, F2 red for discard while editing, F4 amber for exit. Use single mode-aware LED manager. Timing-sensitive LED update with MIDI RX must be tested (NeoPixel driver may mask interrupts); safe LED update scheduling or hardware-compatible alternative.

## Configuration format and desktop tool
- Native **Python application in this same repository**, not a TypeScript or browser editor. PySide6 GUI and pyserial suggested; keep JSON schema, validation, compact binary serialization and transport independent of GUI. Include CLI commands `validate`, `upload/apply`, `download`, `save`, `reload`, and tests. User-friendly visual 4x4 pad matrix/page tabs and editors for each physical control.
- JSON files in repo for editable source configs; binary compact versioned representation over serial/EEPROM. Define explicit IDs/types/enums, valid ranges, schema and checksum; preserve forwards/backwards compatibility intentionally. Commands should identify firmware/protocol, read live working config, apply whole config atomically to RAM, explicitly persist EEPROM, reload saved config, status, and optional monitoring. Protect against serial disconnect or partial uploads. Document whether apply updates output live and handles active notes.
- On-device editing must use same config model as Python app. Saving to EEPROM is never automatic. Track configuration dirty state. Do not assume 2 full EEPROM slots will fit before calculating actual record sizes. Include bounded memory budget (RAM, EEPROM, flash) and build reports.
- Runtime toggle and encoder values, pickup, active notes and selected page are not automatically persisted; configuration may include initial values and page labels.

## Engineering expectations
1. Read README, schematic as needed, actual source and PlatformIO config first. Preserve working IO test behavior via separate diagnostic mode or documented bench tests if possible.
2. Break implementation into coherent modules: hardware scan/events; modifier/page/mode state machine; mappings/action dispatch; MIDI passthrough & merge; analog filtering/pickup; LCD/LED; config persistence; serial protocol; Python GUI/CLI. Avoid monolithic main.cpp and dynamic allocation/large String usage.
3. Include unit tests where possible (host-side pure state machines, encode/decode, config CRC, packet framing, pickup, message merger), hardware bench checklist and build/usage docs.
4. Validate full `pio run -d firmware`, Python tests, and report memory use and any unverified hardware behavior.
5. Implement in reviewable steps/commits. Do not use unused global shift slots for system functions without explicit user design. Do not assume hardware is fabricated or connected.
6. If a specific menu layout or command has no agreed behavior, choose small documented defaults but do not invent irreversible or hazardous actions.

## Acceptance criteria
- Hardware pinout matches README including analog A12/A14; MIDI RX D19 TX D18 remain functional.
- Incoming MIDI clock, transport, notes, CC, program changes and SysEx survive passthrough while locally generated MIDI is emitted, without corruption.
- F1–F4 pages work; F4+pad16 enters assignment selection; selection & encoder editing & F1 RAM apply/F2 discard/F4 exit work, normal mappings suppressed while editing.
- Proper per-page mapping behavior and fader pickup; no hanging notes on page/mode changes.
- Four APA106-compatible LEDs indicate mode and actions; 20×2 LCD displays page/BPM and recent control output.
- Configurator edits all 128 slots, can apply, download and explicitly save config; no EEPROM writes unless requested.
- Test/build directions and limitations documented.

## Suggested sequencing
A. Build clean hardware event + MIDI transport core and diagnostics. B. Page/modifier mode & LED LCD. C. Mapping execution, pickup, note safety. D. Assignment editor + RAM config. E. EEPROM and framed serial protocol. F. Python CLI + GUI + tests and documentation.
