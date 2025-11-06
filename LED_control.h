//
// Created by cheb2 on 11/5/2025.
//

#ifndef GM_CLOCK_LED_CONTROL_H
#define GM_CLOCK_LED_CONTROL_H

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <cstdint>
#include <vector>

// === SPI Config ===
#define SPI_PORT spi1
#define PIN_SDI 1
#define PIN_CKI 0

// === LED Config ===
#define NUM_LEDS 132

// === Structs ===
struct RGB {
    uint8_t r, g, b;
};

void led_init();
void send_led_frame(spi_inst_t *spi, uint8_t brightness, const RGB &color,  int index);
void apa102_LED_show(const std::vector<RGB> &leds, uint8_t brightness = 31);

#endif //GM_CLOCK_LED_CONTROL_H