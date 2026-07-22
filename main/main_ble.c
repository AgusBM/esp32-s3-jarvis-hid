#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "device/usbd.h"
#include "class/hid/hid_device.h"
#include "soc/usb_serial_jtag_struct.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "main_ble.h"

static const char *TAG = "usb_hid";

/************* TinyUSB descriptors ****************/

#define TUSB_DESC_TOTAL_LEN      (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

/**
 * @brief HID report descriptor (keyboard only, no Report ID)
 */
const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(1))
};

/**
 * @brief Configuration descriptor
 */
static const uint8_t hid_configuration_descriptor[] = {
    /* Configuration number, interface count, string index, total length, attribute, power in mA */
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    /* Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval */
    TUD_HID_DESCRIPTOR(0, 0, false, sizeof(hid_report_descriptor), 0x81, 16, 10),
};

/********* TinyUSB HID callbacks ***************/

/* Invoked when received GET HID REPORT DESCRIPTOR request */
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void) instance;
    return hid_report_descriptor;
}

/* Invoked when received GET_REPORT control request */
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;
    return 0;
}

/* Invoked when received SET_REPORT control request */
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) bufsize;
}

/********* Application ***************/

void usb_hid_init(void)
{
    ESP_LOGI(TAG, "Initializing USB HID keyboard");

    // Force a visible USB disconnect before handing the internal PHY from
    // USB Serial/JTAG to TinyUSB. Without this pause, some hosts retain the
    // ROM 303a:1001 enumeration instead of probing the HID descriptors.
    USB_SERIAL_JTAG.conf0.pad_pull_override = 1;
    USB_SERIAL_JTAG.conf0.dp_pullup = 0;
    USB_SERIAL_JTAG.conf0.usb_pad_enable = 0;
    vTaskDelay(pdMS_TO_TICKS(25));

    const tinyusb_config_t tusb_cfg = {
        .port = TINYUSB_PORT_FULL_SPEED_0,
        .phy = {
            .skip_setup = false,
            .self_powered = false,
            .vbus_monitor_io = 0,
        },
        .task = {
            .size = 4096,
            .priority = 5,
            .xCoreID = 0,
        },
        .descriptor = {
            .device = NULL,
            .qualifier = NULL,
            .string = NULL,
            .string_count = 0,
            .full_speed_config = hid_configuration_descriptor,
            .high_speed_config = NULL,
        },
        .event_cb = NULL,
        .event_arg = NULL,
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "USB HID ready");
}

bool ble_is_ready(void)
{
    return tud_mounted();
}

bool ble_is_connected(void)
{
    return tud_mounted();
}

/* HID keycodes */
#define KEY_NONE     0x00
#define KEY_ENTER    0x28
#define KEY_SPACE    0x2C
#define KEY_A        0x04
#define KEY_B        0x05
#define KEY_C        0x06
#define KEY_E        0x08
#define KEY_H        0x0B
#define KEY_I        0x0C
#define KEY_L        0x0F
#define KEY_M        0x10
#define KEY_N        0x11
#define KEY_O        0x12
#define KEY_R        0x15
#define KEY_S        0x16
#define KEY_T        0x17
#define KEY_U        0x18
#define KEY_V        0x19
#define KEY_Y        0x1C

#define MOD_LCTRL    0x01
#define MOD_LSHIFT   0x02
#define MOD_LALT     0x04
#define MOD_LGUI     0x08

static void send_report(uint8_t mod, uint8_t key)
{
    if (!tud_mounted()) {
        ESP_LOGW(TAG, "USB not mounted, skip key");
        return;
    }

    uint8_t keycode[6] = {0};

    /* Press modifier first (if any) */
    if (mod != 0) {
        tud_hid_keyboard_report(1, mod, keycode);
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    /* Press key (with modifier held) */
    keycode[0] = key;
    tud_hid_keyboard_report(1, mod, keycode);
    ESP_LOGI(TAG, "key 0x%02x press -> OK", key);
    vTaskDelay(pdMS_TO_TICKS(40));

    /* Release all */
    tud_hid_keyboard_report(1, 0, NULL);
    ESP_LOGI(TAG, "key 0x%02x release -> OK", key);
    vTaskDelay(pdMS_TO_TICKS(40));
}

void ble_send_ctrl_b(void)
{
    if (!ble_is_ready()) {
        ESP_LOGW(TAG, "USB not ready, skip macro");
        return;
    }

    ESP_LOGI(TAG, "Sending macro: CTRL+B");
    send_report(MOD_LCTRL, KEY_B);
    ESP_LOGI(TAG, "CTRL+B sent");
}

void ble_delayed_init_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "Starting USB HID");
    usb_hid_init();
    vTaskDelete(NULL);
}
