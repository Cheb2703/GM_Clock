//
// Created by cheb2 on 11/5/2025.
//

/* LED SetUp
 * #1 : Bottom Left Corner LED
 * #2-33 : First Digit
 * #34-65 : Second Digit
 * #66 & #67 : Colon
 * #68 - 99 : Third Digit
 * #100 - 131 : Fourth Digit
 * #132 : Top Right Corner
 *
 * #1 Digit Pin 1 (SDI) on GPIO1
 *          Pin 2 (CKI) on GPIO0
 */

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <vector>
#include "LED_control.h"
#include <cstdio>
#include <strings.h>


void led_init() {
    spi_init(SPI_PORT, 4000000); // 4 MHz
    gpio_set_function(PIN_SDI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_CKI, GPIO_FUNC_SPI);
}


void send_led_frame(spi_inst_t *spi, uint8_t brightness, const RGB &color, int index) {
    uint8_t frame[4];
    frame[0] = 0b11100000 | (brightness & 0b00011111); // Brightness
    frame[1] = color.b;
    frame[2] = color.g;
    frame[3] = color.r;

    // Simulate SPI transmission (print info)
    printf("LED #%d | Brightness: %u | R: %3u G: %3u B: %3u | Frame: [%02X %02X %02X %02X]\n",
           index, brightness, color.r, color.g, color.b,
           frame[0], frame[1], frame[2], frame[3]);


    spi_write_blocking(spi, frame, 4);
}

void apa102_LED_show(const std::vector<RGB> &leds, const uint8_t brightness) {
    // Start frame
    constexpr  uint8_t start[4] = {0x00, 0x00, 0x00, 0x00};
    spi_write_blocking(SPI_PORT, start, 4);
    printf("=== START FRAME ===\n");

    // LED frames
    for (int i = 0; i < leds.size(); i++)
        send_led_frame(SPI_PORT, brightness, leds[i], i);

    // End frame
    constexpr uint8_t end[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    spi_write_blocking(SPI_PORT, end, 4);
    printf("=== END FRAME ===\n\n");

}