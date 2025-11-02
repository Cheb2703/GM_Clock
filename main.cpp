#include <cstdio>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "wifi_setup.h"
#include <lwip/dns.h>


int main() {
    stdio_init_all();  // Initialize USB stdio
    sleep_ms(2000);

    if (connect_to_wifi() == 0) {
        printf("Wi-Fi connected successfully!\n");
    } else {
        printf("Wi-Fi connection failed!\n");
    }
}

