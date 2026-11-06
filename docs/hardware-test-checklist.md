# MIDIcon v1 hardware bench checklist

- Verify the board boots from USB with all peripherals attached and the 5 V rail remains stable.
- Exercise every matrix key, encoder direction/switch, function button, fader, and all four joystick axes.
- Confirm F1–F4 taps select pages and a consumed function+matrix chord never changes page.
- Confirm only F4+pad 1 and F4+pad 16 enter modes; all other 62 chords remain inert.
- Hold and release mapped notes across page/menu transitions; verify no stuck notes.
- Verify momentary, toggle, trigger, absolute, relative, stepped, Program Change, and pitch bend mappings.
- Check fader pickup after page changes and after holding a function button; confirm joysticks return to center normally.
- Calibrate all eight analog controls, save, power-cycle, and verify restoration. Interrupt power during a save and verify one EEPROM slot remains recoverable.
- Test SAVE, dirty LOAD confirmation, invalid/blank EEPROM, and repeated save of unchanged configuration.
- Confirm APA106 red/green/blue ordering, page indication, and mode-specific F1/F2/F4 colors.
- Pass notes, pressure, Program Change, pitch bend, system common, long SysEx, clock, and transport through MIDI IN.
- During long SysEx, move controls and verify local messages do not corrupt the SysEx stream.
- Run dense MIDI clock plus rapid analog movement and check for dropped clocks/UART overflow.
- Verify BPM smoothing, clock timeout, Start/Continue/Stop indication, and absence of generated MIDI clock.
- Upload/download with the configurator, unplug it during transfer, and verify the previous working configuration remains active.
- Confirm standalone operation after disconnecting USB serial and power-cycling.
