#ifndef GPIO_CONTROL_H
#define GPIO_CONTROL_H

#include "driver/gpio.h"

// GPIO Pin Definitions
#define BUTTON_GPIO         GPIO_NUM_16 // GPIO for button
#define LED_NOTIFICATION    GPIO_NUM_2  // GPIO for notification LED
#define LED_GREEN           GPIO_NUM_15 // GPIO for green LED
#define LED_RED            GPIO_NUM_13 // GPIO for red LED
#define LED_YELLOW         GPIO_NUM_12 // GPIO for yellow LED

// LED Control Macros
#define LED_NOTIFICATION_ON  gpio_set_level(LED_NOTIFICATION, 1)
#define LED_NOTIFICATION_OFF gpio_set_level(LED_NOTIFICATION, 0)
#define LED_GREEN_ON        gpio_set_level(LED_GREEN, 1)
#define LED_GREEN_OFF       gpio_set_level(LED_GREEN, 0)
#define LED_RED_ON          gpio_set_level(LED_RED, 1)
#define LED_RED_OFF         gpio_set_level(LED_RED, 0)
#define LED_YELLOW_ON       gpio_set_level(LED_YELLOW, 1)
#define LED_YELLOW_OFF      gpio_set_level(LED_YELLOW, 0)
#define SET_ALL_LED(__lg__, __lr__, __ly__) { \
    gpio_set_level(LED_GREEN, (__lg__)); \
    gpio_set_level(LED_RED, (__lr__)); \
    gpio_set_level(LED_YELLOW, (__ly__)); \
}

void configure_gpio(void);

#endif /* GPIO_CONTROL_H */
