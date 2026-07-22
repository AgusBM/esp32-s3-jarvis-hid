#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "esp_board_init.h"
#include "model_path.h"
#include "string.h"
#include "driver/rmt.h"
#include "soc/rmt_struct.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "main_ble.h"

static const char *TAG = "main";

#define RGB_LED_GPIO    48
#define RMT_CHANNEL     RMT_CHANNEL_0

#define T0H   32
#define T0L   68
#define T1H   68
#define T1L   32
#define RESET_US 60

static volatile bool led_vad_active = false;
static volatile bool led_wake_pending = false;
static const esp_afe_sr_iface_t *afe_handle = NULL;
static volatile int task_flag = 0;
static volatile bool audio_active = false;
#define AUDIO_ENERGY_THRESHOLD  6000
#define SILENCE_TIMEOUT_US      (2LL * 1000 * 1000)
/* vadnet1_medium keeps SPEECH active for about 992 ms of trailing noise.
 * Wait only the remaining interval so Ctrl+B occurs about 2 s after
 * the user's actual last spoken audio, rather than about 3 s later. */
#define VAD_TRAILING_SPEECH_US  (992LL * 1000)
#define SILENCE_CONFIRM_US      (SILENCE_TIMEOUT_US - VAD_TRAILING_SPEECH_US)

static void ws2812_init(void)
{
    rmt_config_t config = {
        .rmt_mode = RMT_MODE_TX,
        .channel = RMT_CHANNEL,
        .gpio_num = RGB_LED_GPIO,
        .clk_div = 1,
        .mem_block_num = 1,
        .tx_config = {
            .carrier_en = false,
            .loop_en = false,
            .idle_output_en = true,
            .idle_level = RMT_IDLE_LEVEL_LOW,
        }
    };
    rmt_config(&config);
    rmt_driver_install(config.channel, 0, 0);
}

static void ws2812_set_color(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t grb[3] = { g, r, b };
    rmt_item32_t items[24];
    for (int i = 0; i < 24; i++) {
        int byte_idx = i / 8;
        int bit_idx = 7 - (i % 8);
        bool bit = (grb[byte_idx] >> bit_idx) & 1;
        items[i].duration0 = bit ? T1H : T0H;
        items[i].level0 = 1;
        items[i].duration1 = bit ? T1L : T0L;
        items[i].level1 = 0;
    }
    rmt_write_items(RMT_CHANNEL, items, 24, 1);
    rmt_wait_tx_done(RMT_CHANNEL, portMAX_DELAY);
    esp_rom_delay_us(RESET_US);
}

static void ws2812_off(void)
{
    ws2812_set_color(0, 0, 0);
}

static void boot_animation(void)
{
    int steps = 50;
    for (int i = 0; i <= steps; i++) {
        uint8_t val = (255 * i) / steps;
        ws2812_set_color(0, val, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    for (int i = steps; i >= 0; i--) {
        uint8_t val = (255 * i) / steps;
        ws2812_set_color(0, val, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ws2812_off();
}

static void mic_ok_animation(void)
{
    int steps = 50;
    for (int i = 0; i <= steps; i++) {
        uint8_t val = (255 * i) / steps;
        ws2812_set_color(val, val * 0.4, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    for (int i = steps; i >= 0; i--) {
        uint8_t val = (255 * i) / steps;
        ws2812_set_color(val, val * 0.4, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ws2812_off();
}

static void led_Task(void *arg)
{
    (void)arg;
    int vad_cooldown = 0;

    while (1) {
        if (led_wake_pending) {
            led_wake_pending = false;
            led_vad_active = false;
            for (int f = 0; f < 3; f++) {
                ws2812_set_color(0, 0, 64);
                vTaskDelay(pdMS_TO_TICKS(80));
                ws2812_off();
                vTaskDelay(pdMS_TO_TICKS(80));
            }
            vad_cooldown = 20;
        } else if (led_vad_active && vad_cooldown == 0) {
            ws2812_set_color(48, 48, 48);
            vTaskDelay(pdMS_TO_TICKS(80));
            ws2812_off();
            vad_cooldown = 10;
        }

        if (vad_cooldown > 0) vad_cooldown--;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void feed_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_feed_channel_num(afe_data);
    int feed_channel = esp_get_feed_channel();
    assert(nch == feed_channel);
    int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    assert(i2s_buff);

    while (task_flag) {
        esp_get_feed_data(true, i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel);

        int samples = audio_chunksize * feed_channel;
        int64_t energy = 0;
        for (int i = 0; i < samples; i++) {
            int16_t s = i2s_buff[i];
            if (s < 0) s = -s;
            energy += s;
        }
        int64_t avg = energy / samples;
        audio_active = (avg > AUDIO_ENERGY_THRESHOLD);

        static int debug_cnt = 0;
        if (++debug_cnt % 20 == 0) {
            printf("audio energy avg=%lld active=%d\n", avg, audio_active);
        }

        afe_handle->feed(afe_data, i2s_buff);
    }
    if (i2s_buff) {
        free(i2s_buff);
        i2s_buff = NULL;
    }
    vTaskDelete(NULL);
}

void detect_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);
    int16_t *buff = malloc(afe_chunksize * sizeof(int16_t));
    assert(buff);
    printf("------------detect start------------\n");

    afe_handle->set_wakenet_threshold(afe_data, 1, 0.3);
    afe_handle->set_wakenet_threshold(afe_data, 2, 0.3);

    bool listening = false;
    int64_t last_voice_us = 0;

    while (task_flag) {
        afe_fetch_result_t *res = afe_handle->fetch(afe_data);
        if (!res || res->ret_value == ESP_FAIL) {
            printf("fetch error!\n");
            break;
        }

        int64_t now_us = esp_timer_get_time();

        if (res->wakeup_state == WAKENET_DETECTED) {
            printf("wakeword detected\n");
            printf("model index:%d, word index:%d\n", res->wakenet_model_index, res->wake_word_index);

            if (!listening) {
                printf("-----------LISTENING-----------\n");
                led_wake_pending = true;
                ble_send_ctrl_b();
                listening = true;
                last_voice_us = now_us;
            } else {
                printf("wakeword ignored: already listening\n");
            }
        } else {
            led_vad_active = audio_active;
        }

        if (listening) {
            if (res->vad_state == VAD_SPEECH) {
                last_voice_us = now_us;
            } else if ((now_us - last_voice_us) >= SILENCE_CONFIRM_US) {
                listening = false;
                printf("-----------2s SILENCE: STOP LISTENING-----------\n");
                ble_send_ctrl_b();
                led_wake_pending = true;
            }
        }
    }
    if (buff) {
        free(buff);
        buff = NULL;
    }
    vTaskDelete(NULL);
}

void app_main()
{
    ESP_LOGI(TAG, "Initializing RGB LED on GPIO%d", RGB_LED_GPIO);
    ws2812_init();
    boot_animation();

    esp_err_t board_ret = esp_board_init(16000, 1, 16);
    if (board_ret == ESP_OK) {
        mic_ok_animation();
        ESP_LOGI(TAG, "Board + mic initialized OK");
    } else {
        ESP_LOGE(TAG, "Board init failed!");
    }

    srmodel_list_t *models = esp_srmodel_init("model");
    if (models) {
        for (int i = 0; i < models->num; i++) {
            if (strstr(models->model_name[i], ESP_WN_PREFIX) != NULL) {
                printf("wakenet model in flash: %s\n", models->model_name[i]);
            }
        }
    }

    afe_config_t *afe_config = afe_config_init(esp_get_input_format(), models, AFE_TYPE_SR, AFE_MODE_LOW_COST);

    if (afe_config->wakenet_model_name) {
        printf("wakeword model in AFE config: %s\n", afe_config->wakenet_model_name);
    }
    if (afe_config->wakenet_model_name_2) {
        printf("wakeword model in AFE config: %s\n", afe_config->wakenet_model_name_2);
    }

    afe_handle = esp_afe_handle_from_config(afe_config);
    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(afe_config);

    int nch = afe_handle->get_feed_channel_num(afe_data);
    int feed_channel = esp_get_feed_channel();
    printf("AFE expects %d channels, board provides %d\n", nch, feed_channel);
    assert(nch == feed_channel);

    afe_config_free(afe_config);

    task_flag = 1;
    xTaskCreatePinnedToCore(&led_Task, "led", 2 * 1024, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(&feed_Task, "feed", 8 * 1024, (void *)afe_data, 5, NULL, 0);
    xTaskCreatePinnedToCore(&detect_Task, "detect", 4 * 1024, (void *)afe_data, 5, NULL, 1);

    /* USB HID init (no delay needed, no BT to coexist with) */
    usb_hid_init();
}
