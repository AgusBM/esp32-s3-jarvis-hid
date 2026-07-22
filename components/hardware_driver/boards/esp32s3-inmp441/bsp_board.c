/*
   Custom board for generic ESP32-S3 + INMP441 I2S MEMS microphone
   Based on esp32s3-eye board implementation.
*/
#include "string.h"
#include "stdlib.h"
#include "bsp_board.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "driver/i2s_std.h"
#include "driver/i2s_tdm.h"
#include "soc/soc_caps.h"
#else
#include "driver/i2s.h"
#endif
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

#define ADC_I2S_CHANNEL 1
static const char *TAG = "board";

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
static i2s_chan_handle_t rx_handle = NULL;
#endif

static esp_err_t bsp_i2s_init(i2s_port_t i2s_num, uint32_t sample_rate, int channel_format, int bits_per_chan)
{
    esp_err_t ret_val = ESP_OK;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(i2s_num, I2S_ROLE_MASTER);
    ret_val |= i2s_new_channel(&chan_cfg, NULL, &rx_handle);
    i2s_std_config_t std_cfg = I2S_CONFIG_DEFAULT(16000, I2S_SLOT_MODE_MONO, 32);
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    ret_val |= i2s_channel_init_std_mode(rx_handle, &std_cfg);
    ret_val |= i2s_channel_enable(rx_handle);
#else
    i2s_config_t i2s_config = I2S_CONFIG_DEFAULT(sample_rate, I2S_CHANNEL_FMT_ONLY_LEFT, bits_per_chan);
    i2s_pin_config_t pin_config = {
        .bck_io_num = GPIO_I2S_SCLK,
        .ws_io_num = GPIO_I2S_LRCK,
        .data_out_num = GPIO_I2S_DOUT,
        .data_in_num = GPIO_I2S_SDIN,
        .mck_io_num = GPIO_I2S_MCLK,
    };
    ret_val |= i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
    ret_val |= i2s_set_pin(i2s_num, &pin_config);
#endif
    return ret_val;
}

static esp_err_t bsp_i2s_deinit(i2s_port_t i2s_num)
{
    esp_err_t ret_val = ESP_OK;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    ret_val |= i2s_channel_disable(rx_handle);
    ret_val |= i2s_del_channel(rx_handle);
    rx_handle = NULL;
#else
    ret_val |= i2s_stop(i2s_num);
    ret_val |= i2s_driver_uninstall(i2s_num);
#endif
    return ret_val;
}

esp_err_t bsp_get_feed_data(bool is_get_raw_channel, int16_t *buffer, int buffer_len)
{
    esp_err_t ret = ESP_OK;
    size_t bytes_read;

    // I2S entrega muestras de 32 bits; el buffer de salida espera int16_t
    // Necesitamos leer el doble de bytes para llenar las muestras de 32 bits
    static int32_t *raw_buff = NULL;
    static int raw_buff_len = 0;
    if (raw_buff_len < buffer_len * 2) {
        free(raw_buff);
        raw_buff = malloc(buffer_len * 2);
        raw_buff_len = buffer_len * 2;
    }
    if (!raw_buff) {
        return ESP_ERR_NO_MEM;
    }

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    ret = i2s_channel_read(rx_handle, raw_buff, buffer_len * 2, &bytes_read, portMAX_DELAY);
#else
    ret = i2s_read(I2S_NUM_1, raw_buff, buffer_len * 2, &bytes_read, portMAX_DELAY);
#endif

    int samples = bytes_read / sizeof(int32_t);
    for (int i = 0; i < samples; i++) {
        // Amplificar desplazando 11 bits y empaquetar a 16 bits real
        buffer[i] = (int16_t)(raw_buff[i] >> 11);
    }

    return ret;
}

int bsp_get_feed_channel(void)
{
    return ADC_I2S_CHANNEL;
}

char* bsp_get_input_format(void)
{
    return "M";
}

esp_err_t bsp_board_init(uint32_t sample_rate, int channel_format, int bits_per_chan)
{
    ESP_LOGI(TAG, "Initializing custom ESP32-S3 + INMP441 board");
    bsp_i2s_init(I2S_NUM_1, 16000, 2, 32);
    return ESP_OK;
}

esp_err_t bsp_sdcard_init(char *mount_point, size_t max_files)
{
    ESP_LOGW(TAG, "SD card not supported on custom INMP441 board");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bsp_sdcard_deinit(char *mount_point)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bsp_audio_play(const int16_t *data, int length, TickType_t ticks_to_wait)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bsp_audio_set_play_vol(int volume)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bsp_audio_get_play_vol(int *volume)
{
    return ESP_ERR_NOT_SUPPORTED;
}
