#include "gpio_control.h"
#include "esp_log.h"

static const char *TAG = "GPIO";

esp_err_t init_gpio(void) {
    // Configure GPIO for button
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PAIR_CFG) | (1ULL << BUTTON_SEND_SMS),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // Configure GPIO for LEDs
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_NOTIFICATION) | (1ULL << LED_GREEN) | 
                        (1ULL << LED_RED) | (1ULL << LED_YELLOW),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };    esp_err_t ret = gpio_config(&led_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LED GPIOs");
        return ret;
    }
    
    ESP_LOGI(TAG, "GPIO initialized");
    return ESP_OK;
}
