#pragma once

#include <stdbool.h>
#include "esp_err.h"

void      usb_hid_init(void);
bool      ble_is_ready(void);
bool      ble_is_connected(void);
void      ble_send_ctrl_b(void);
void      ble_delayed_init_task(void *arg);
