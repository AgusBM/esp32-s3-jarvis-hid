# ESP32-S3 Jarvis HID

Hands-free voice control firmware for an ESP32-S3. It detects the wake word **Jarvis**, sends `Ctrl+B` over native USB HID to start voice input, and sends `Ctrl+B` again after roughly two seconds without speech.

## Hardware

- ESP32-S3 with native USB/OTG
- INMP441 I2S microphone
- WS2812 RGB LED

### Wiring

| Device | ESP32-S3 GPIO |
|---|---:|
| INMP441 SD | 8 |
| INMP441 SCK/BCLK | 9 |
| INMP441 WS/LRCLK | 39 |
| WS2812 data | 48 |

The INMP441 L/R pin is configured for the left I2S slot used by the firmware.

## Behavior

1. WakeNet detects `Jarvis`.
2. The device sends `Ctrl+B` as a USB HID keyboard.
3. ESP-SR's neural VAD tracks speech rather than raw ambient volume.
4. After about two seconds without speech, the device sends a second `Ctrl+B`.
5. Additional wake-word detections are ignored while the listening session is active.

A second WakeNet model (`Hi Lexin`) is included by the current configuration.

## Software

- ESP-IDF v5.0.8
- ESP-SR / WakeNet 9
- `vadnet1_medium`
- Espressif TinyUSB HID

The custom INMP441 board support is included under `components/hardware_driver`, so the project does not depend on an absolute local ESP-Skainet checkout.

## Build

Install and export ESP-IDF v5.0.8, then run:

```bash
idf.py set-target esp32s3
idf.py build
```

## Flash

Use the UART/programming port for flashing:

```bash
idf.py -p /dev/ttyACM0 flash
```

The port may differ on your system. Connect the ESP32-S3 native USB/OTG port to the computer that should receive the HID keyboard shortcut.

## Verification

At boot, the serial log should show:

- the INMP441 board initialized
- `wn9_jarvis_tts` loaded
- `vadnet1_medium` in the AFE pipeline
- TinyUSB HID ready

When triggered, the log reports `LISTENING`, followed by `2s SILENCE: STOP LISTENING` after speech ends.
