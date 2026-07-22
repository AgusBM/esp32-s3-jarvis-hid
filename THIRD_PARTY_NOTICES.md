# Third-party software notices

This project builds on software developed by Espressif Systems and other open-source contributors. The versions below are pinned in `dependencies.lock`.

The project license in the root `LICENSE` file applies only to original project code unless a file or component states otherwise. Third-party software remains subject to its own copyright notices and license terms.

## Direct dependencies

### ESP-SR 2.4.7

- Copyright: Espressif Systems (Shanghai) PTE LTD
- Source: https://github.com/espressif/esp-sr
- License: Espressif MIT License
- Local license copy: [`licenses/ESP-SR-LICENSE.txt`](licenses/ESP-SR-LICENSE.txt)

Important: this is not the standard MIT license. Its permission grant is limited to use on Espressif Systems products. The WakeNet and VAD models included in a compiled firmware are subject to these terms.

### ESP TinyUSB 2.2.1

- Copyright: Espressif Systems and contributors
- Source: https://github.com/espressif/esp-usb/tree/master/device/esp_tinyusb
- License: Apache License 2.0
- License text: [`LICENSE`](LICENSE)

## Transitive dependencies

### TinyUSB 0.21.0~1

- Copyright: 2012-2026 hathach (tinyusb.org) and contributors
- Source: https://github.com/hathach/tinyusb
- License: MIT
- Local license copy: [`licenses/TinyUSB-LICENSE.txt`](licenses/TinyUSB-LICENSE.txt)

### ESP-DSP 1.8.0

- Copyright: Espressif Systems and contributors
- Source: https://github.com/espressif/esp-dsp
- License: Apache License 2.0
- License text: [`LICENSE`](LICENSE)

### dl_fft 0.6.0

- Copyright: Espressif Systems and contributors
- Source: https://github.com/espressif/esp-dl/tree/master/esp-dl/tools/dl_fft
- Registry metadata declares MIT; individual source files may carry Apache-2.0 notices. The per-file notice takes precedence where present.

### cJSON 1.7.19~2

- Copyright: 2009-2017 Dave Gamble and cJSON contributors
- Source: https://github.com/DaveGamble/cJSON
- License: MIT
- Local license copy: [`licenses/cJSON-LICENSE.txt`](licenses/cJSON-LICENSE.txt)

## Build framework

### ESP-IDF 5.0.8

- Copyright: Espressif Systems and contributors
- Source: https://github.com/espressif/esp-idf
- Primary license: Apache License 2.0
- ESP-IDF also contains third-party components under their respective licenses.

ESP-IDF is required to build this project but is not vendored in this repository. Consult its own license files when redistributing ESP-IDF itself.

## Adapted board-support files

Files under `components/hardware_driver` include code adapted from Espressif examples. Existing Espressif copyright and Apache-2.0 notices are retained in those files.

## Redistribution

If you distribute compiled firmware, preserve these notices and include the applicable third-party license texts with the release. If dependencies are upgraded, review their license files again rather than assuming the terms are unchanged.

This notice is provided for attribution and license-tracking purposes and is not legal advice.
