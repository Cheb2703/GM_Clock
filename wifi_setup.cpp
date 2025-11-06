//
// Created by cheb2 on 11/1/2025.
//

#include "wifi_setup.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include <cstdio>
#include <cstring>
#include "lwip/netif.h"

// ------------------ Wi-Fi credentials ------------------
struct WifiNetwork {
    const char* ssid;
    const char* password;
};

// Add as many as you want here
static WifiNetwork networks[] = {

    {"Tailwinds", "planetearth"},
    {"Papa", "planetearth"},
    {"McCormick Air Center", "ykminternet"}

};

static constexpr int NUM_NETWORKS = sizeof(networks) / sizeof(networks[0]);

// ------------------ Internal state ------------------
static bool wifi_initialized = false;
static int current_network = -1;

// ------------------ Wi-Fi Connect Attempt ------------------
static bool try_connect(const char* ssid, const char* password) {
    printf("Attempting to connect to SSID: %s\n", ssid);

    // Try connecting with a timeout (20 seconds)
    int result = cyw43_arch_wifi_connect_timeout_ms(
        ssid, password, CYW43_AUTH_WPA2_AES_PSK, 10000
    );

    if (result == 0) {
        printf("Connected to Wi-Fi network: %s\n", ssid);
        struct netif* netif = &cyw43_state.netif[CYW43_ITF_STA];
        printf("IP address: %s\n", ip4addr_ntoa(netif_ip4_addr(netif)));

        // Solid LED on once connected
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);

        return true;
    } else {
        printf("Failed to connect to %s (error %d)\n", ssid, result);
        return false;
    }
}

// ------------------ Public API ------------------
int connect_to_wifi() {
    printf("Initializing Wi-Fi...\n");

    if (!wifi_initialized) {
        if (cyw43_arch_init()) {
            printf("Failed to initialize Wi-Fi hardware.\n");
            return -1;
        }
        wifi_initialized = true;
        cyw43_arch_enable_sta_mode();
        sleep_ms(500);
    }

    // Keep trying all networks until one succeeds
    while (true) {
        for (int i = 0; i < NUM_NETWORKS; i++) {
            if (try_connect(networks[i].ssid, networks[i].password)) {
                current_network = i;
                printf("Wi-Fi connected successfully to %s\n", networks[i].ssid);
                return 0;
            }
        }
        printf("All networks failed — retrying in 5 seconds...\n");
        sleep_ms(5000);
    }
}

// ------------------ Maintain Connection ------------------
void maintain_wifi_connection() {
    cyw43_arch_poll();

    if (!wifi_initialized)
        return;

    // Already connected
    if (cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_UP)
        return;

    printf("Wi-Fi lost — attempting reconnect...\n");

    // Try reconnecting current network first
    if (current_network >= 0 && try_connect(networks[current_network].ssid, networks[current_network].password)) {
        printf("Reconnected to %s\n", networks[current_network].ssid);
        return;
    }

    // Otherwise, try all networks
    for (int i = 0; i < NUM_NETWORKS; i++) {
        if (try_connect(networks[i].ssid, networks[i].password)) {
            current_network = i;
            printf("Reconnected successfully to %s\n", networks[i].ssid);
            return;
        }
    }

    printf("Reconnect attempt failed — will retry later.\n");
}