#include "config.h"

const char *CONFIG_TAG = "==> config";

config_t config = {
    .board_mode = BOARD_MODE_NONE,
    .orientation = ORIENTATION_NONE,
};

bool read_pin(int pin){
    int pin_state;

    // enable gpio pin
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << pin),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
    };
    gpio_config(&io_conf);

    // get pin state
    pin_state = gpio_get_level(pin);

    // disable gpio pin
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);
    return pin_state == 0? false : true;
}

board_mode_t read_board_mode(char config_bits) {
    return config_bits >> 2 == 0? BOARD_MODE_PEER_LINK : (board_mode_t) config_bits;
}

orientation_t read_orientation(char config_bits) {
    return config_bits >> 2 == 0? (orientation_t) config_bits : ORIENTATION_NONE;
}

void print_board_mode(board_mode_t board_mode) {
    switch(board_mode) {
        case BOARD_MODE_PEER_LINK:
            ESP_LOGI(CONFIG_TAG, "BOARD_MODE_PEER_LINK");
            break;
        case BOARD_MODE_ACCESS_POINT:
            ESP_LOGI(CONFIG_TAG, "BOARD_MODE_ACCESS_POINT (100)");
            break;
        case BOARD_MODE_ROOT:
            ESP_LOGI(CONFIG_TAG, "BOARD_MODE_ROOT (101)");
            break;
        case BOARD_MODE_NONE:
            ESP_LOGI(CONFIG_TAG, "BOARD_MODE_NONE");
            break;
        default:
            ESP_LOGW(CONFIG_TAG, "Invalid Mode (%i)", board_mode);
            break;
    }
}

void print_orientation(orientation_t orientation) {
    switch(orientation) {
        case ORIENTATION_NORTH:
            ESP_LOGI(CONFIG_TAG, "ORIENTATION_NORTH (000)");
            break;
        case ORIENTATION_SOUTH:
            ESP_LOGI(CONFIG_TAG, "ORIENTATION_SOUTH (001)");
            break;
        case ORIENTATION_EAST:
            ESP_LOGI(CONFIG_TAG, "ORIENTATION_EAST (010)");
            break;
        case ORIENTATION_WEST:
            ESP_LOGI(CONFIG_TAG, "ORIENTATION_WEST (011)");
            break;
        case ORIENTATION_NONE:
            ESP_LOGI(CONFIG_TAG, "ORIENTATION_NONE");
            break;
        default:
            ESP_LOGW(CONFIG_TAG, "Invalid Orientation (%i)", orientation);
            break;
    }
}

void print_config(config_t *cfg){
    print_board_mode(cfg->board_mode);
    print_orientation(cfg->orientation);
}

void read_config(void){
    // Initialize config_bits with all zeros
    char config_bits = 0b0;

    // Read GPIO pins
    bool config_bit_0 = read_pin(CONFIG_GPIO_PIN_0);
    bool config_bit_1 = read_pin(CONFIG_GPIO_PIN_1);
    bool config_bit_2 = read_pin(CONFIG_GPIO_PIN_2);

    // Set the corresponding bits in config_bits based on GPIO pin values
    config_bits |= (config_bit_0 << 0);
    config_bits |= (config_bit_1 << 1);
    config_bits |= (config_bit_2 << 2);

    // Set the mode and orientation based on config_bits
    config.board_mode = read_board_mode(config_bits);
    config.orientation = read_orientation(config_bits);

    print_config(&config);
}

board_mode_t get_board_mode(void) {
    return config.board_mode;
}

orientation_t get_orientation(void){
    return config.orientation;
}
