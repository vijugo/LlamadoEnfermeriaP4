#include "codec.h"
#include "touch.h"
#include "esp_log.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "es8311_codec.h"
#include "esp_codec_dev_defaults.h"
#include "audio_player.h"

static const char *TAG = "codec";
static esp_codec_dev_handle_t play_dev_handle = NULL;

static esp_err_t _audio_player_mute(AUDIO_PLAYER_MUTE_SETTING setting) {
    return esp_codec_dev_set_out_mute(play_dev_handle, setting == AUDIO_PLAYER_MUTE);
}

static esp_err_t _audio_player_reconfig(uint32_t rate, uint32_t bits, i2s_slot_mode_t ch) {
    esp_codec_dev_sample_info_t fs = {
        .sample_rate = rate,
        .channel = (ch == I2S_SLOT_MODE_MONO) ? 1 : 2,
        .bits_per_sample = bits,
    };
    return esp_codec_dev_open(play_dev_handle, &fs);
}

static esp_err_t _audio_player_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms) {
    int ret = esp_codec_dev_write(play_dev_handle, audio_buffer, (int)len);
    *bytes_written = (ret == ESP_CODEC_DEV_OK) ? len : 0;
    return (ret == ESP_CODEC_DEV_OK) ? ESP_OK : ESP_FAIL;
}

esp_err_t codec_init(void) {
    ESP_LOGI(TAG, "Initializing Audio Subsystem (ES8311)...");

    i2c_master_bus_handle_t i2c_bus = touch_get_i2c_bus();

    static i2s_chan_handle_t tx_handle = NULL;
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, NULL));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = (gpio_num_t)CODEC_I2S_MCLK,
            .bclk = (gpio_num_t)CODEC_I2S_SCLK,
            .ws = (gpio_num_t)CODEC_I2S_LCLK,
            .dout = (gpio_num_t)CODEC_I2S_DOUT,
            .din = (gpio_num_t)CODEC_I2S_DSIN,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));

    audio_codec_i2c_cfg_t i2c_cfg = { .port = (uint8_t)TOUCH_I2C_PORT, .addr = ES8311_CODEC_DEFAULT_ADDR, .bus_handle = i2c_bus };
    const audio_codec_ctrl_if_t *i2c_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    es8311_codec_cfg_t es8311_cfg = { .ctrl_if = i2c_if };
    const audio_codec_if_t *es8311_if = es8311_codec_new(&es8311_cfg);
    audio_codec_i2s_cfg_t i2s_cfg = { .port = I2S_NUM_0, .rx_handle = NULL, .tx_handle = tx_handle };
    const audio_codec_data_if_t *data_if = audio_codec_new_i2s_data(&i2s_cfg);

    esp_codec_dev_cfg_t dev_cfg = { .codec_if = es8311_if, .data_if = data_if, .dev_type = ESP_CODEC_DEV_TYPE_OUT };
    play_dev_handle = esp_codec_dev_new(&dev_cfg);

    gpio_config_t pa_conf = { .pin_bit_mask = (1ULL << CODEC_PA_ENABLE_GPIO), .mode = GPIO_MODE_OUTPUT };
    gpio_config(&pa_conf);
    gpio_set_level((gpio_num_t)CODEC_PA_ENABLE_GPIO, 1);

    audio_player_config_t player_cfg = {
        .mute_fn = _audio_player_mute,
        .clk_set_fn = _audio_player_reconfig,
        .write_fn = _audio_player_write,
        .priority = 5,
        .coreID = 1
    };
    audio_player_new(player_cfg);

    ESP_LOGI(TAG, "Audio Subsystem ready (Silent mode).");
    return ESP_OK;
}

esp_err_t codec_play_audio_file(int file_number, int volume, int repeats) {
    char path[64];
    snprintf(path, sizeof(path), "/sdcard/%d.mp3", file_number);
    FILE *fp = fopen(path, "rb");
    if (!fp) return ESP_ERR_NOT_FOUND;
    codec_set_volume(volume);
    return audio_player_play(fp);
}

esp_err_t codec_set_volume(int volume) {
    return play_dev_handle ? esp_codec_dev_set_out_vol(play_dev_handle, volume) : ESP_FAIL;
}
