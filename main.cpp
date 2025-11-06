#include <cstdio>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "wifi_setup.h"
#include <lwip/dns.h>
#include "time_grab.h"
#include "hardware/powman.h"
#include "pico/util/datetime.h"
#include "LED_control.h"
#include "hardware/spi.h"
#include <vector>


int main() {
    stdio_init_all();
    sleep_ms(4000);

    printf("Starting Pico 2 W Wi-Fi + NTP test\n");

    if (connect_to_wifi() != 0) {
        printf("Wi-Fi failed. Halting.\n");
        return 1;
    }

    init_ntp_client();

    set_timezone_offset(-8 * 3600); //UTC-8 (PST)

    // --- LED Setup ---
    led_init();
    std::vector<RGB> leds(NUM_LEDS);
    for (auto &led : leds)
        led = {255, 0, 0}; // Red for testing

    absolute_time_t last_print = {0};
    while (true) {

        //keep Wi-Fi working
        maintain_wifi_connection();

        // Poll Wi-Fi / LWIP safely
        poll_ntp();

        // Blink LED every second
        static absolute_time_t last_led = {0};
        if (absolute_time_diff_us(last_led, get_absolute_time()) > 1000000) {
            last_led = get_absolute_time();
            static bool led_on = false;
            led_on = !led_on;
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
        }

        // Print time every 5 seconds if synced
        if (absolute_time_diff_us(last_print, get_absolute_time()) > 1000000) {
            last_print = get_absolute_time();
            if (is_time_synced()) {
                char buf[32];
                get_current_time_str(buf, sizeof(buf));
                printf("Current date & time: %s\n", buf);
            } else {
                printf("Time not synced yet.\n");
            }
        }

        sleep_ms(10);  // tiny delay

        // LED Code----------

        apa102_LED_show(leds, 15);
        sleep_ms(8000);

    }
}

