# MIDIcon configurator

Python 3 application using PySide6 and pyserial.

```sh
python -m pip install -r requirements.txt
python -m configurator validate config/default.json
python -m configurator --port /dev/ttyUSB0 upload config/default.json
python -m configurator --port /dev/ttyUSB0 download backup.json
python -m configurator --port /dev/ttyUSB0 save
python -m configurator --port /dev/ttyUSB0 reload
python -m configurator gui
```

`upload` applies a complete validated configuration to RAM and does not write EEPROM. `save` is the explicit persistence operation. JSON is the editable/interchange form; the USB representation is the same packed 1634-byte model used by firmware. Global function-button chords are firmware-owned and are intentionally absent from the model.

The GUI provides the four page tabs and matrix action overview. The CLI is the reference interface for complete JSON import/export and device operations.
