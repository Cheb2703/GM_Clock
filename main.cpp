// GM Clock 10/10/2025


//Test Code for IDE & Upload Success

#include "pico/stdlib.h"

int main() {
    const uint LED_PIN = 25; // Built-in LED on Pico
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (true) {
        gpio_put(LED_PIN, 1);
        sleep_ms(500);
        gpio_put(LED_PIN, 0);
        sleep_ms(500);
    }
}
