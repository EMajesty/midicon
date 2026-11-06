# Firmware I/O test

This PlatformIO project targets the Arduino Mega 2560 Pro wiring described in
the repository's main `README.md`.

## Run

```sh
pio run
pio run --target upload
pio device monitor
```

The USB console runs at 115200 baud. It reports encoder movement, debounced
encoder and function-button changes, debounced matrix-button changes, analog
controls that move by at least 8 ADC counts, and received MIDI bytes.

The LCD displays the last digital event, and all four WS2812B LEDs continuously
cycle through colors. UART1 runs at the MIDI rate of 31250 baud; every MIDI byte
received on D19 is logged and echoed to D18 so the input and output circuits can
be tested together. Avoid connecting the MIDI output back to the MIDI input
while another device is also echoing the data, because that creates a feedback
loop.

