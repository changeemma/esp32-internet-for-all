#include "config.h"

static const char* TAG = "==> config";

static device_config_t s_config = {
    .id          = DEVICE_ID_NONE,
    .mode        = DEVICE_MODE_NONE,
    .orientation = DEVICE_ORIENTATION_NONE,
};

static void enable_config_pins(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = DEVICE_CONFIG_PIN_MASK,
        .intr_type    = GPIO_INTR_DISABLE,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
}

static void reset_config_pins(void)
{
    gpio_reset_pin(DEVICE_CONFIG_PIN_0);
    gpio_reset_pin(DEVICE_CONFIG_PIN_1);
    gpio_reset_pin(DEVICE_CONFIG_PIN_2);
}

// Set the corresponding bits in config_bits based on GPIO pin values
static char read_config_bits(void)
{
    // Initialize config_bits with all zeros
    char config_bits = 0b0;

    enable_config_pins();
    config_bits |= gpio_get_level(DEVICE_CONFIG_PIN_0) << 0;
    config_bits |= gpio_get_level(DEVICE_CONFIG_PIN_1) << 1;
    config_bits |= gpio_get_level(DEVICE_CONFIG_PIN_2) << 2;
    reset_config_pins();

    return config_bits;
}

// Set device config based on config_bits values
void device_config_setup(void)
{
    char config_bits = read_config_bits();

    s_config.id = (device_id_t) config_bits;

    if (config_bits >> 2 == 0) {
        s_config.mode = DEVICE_MODE_PEER_LINK;
        s_config.orientation = (device_orientation_t) config_bits;
    } else {
        s_config.mode = (device_mode_t) config_bits;
        s_config.orientation = DEVICE_ORIENTATION_NONE;
    }
}

device_id_t device_config_get_id(void)
{
    return s_config.id;
}

device_mode_t device_config_get_mode(void)
{
    return s_config.mode;
}

device_orientation_t device_config_get_orientation(void)
{
    return s_config.orientation;
}

void device_config_print(void)
{
    ESP_LOGI(TAG, "Board ID: '%i'", device_config_get_id());
    //ESP_LOGI(TAG, "Board Mode: '%s'", device_config_get_id());
    //ESP_LOGI(TAG, "Board Orientation: '%s'", device_config_get_id());
}
