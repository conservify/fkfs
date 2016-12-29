#include <Arduino.h>
#include <SPI.h>

#include "sd_raw.h"
#include "fkfs.h"

#define SD_PIN_CS                                             16

void setup() {
    Serial.begin(119200);

    // LoRa radio's CS.
    pinMode(8, OUTPUT);
    digitalWrite(8, HIGH);

    while (!Serial) {
        delay(100);
    }

    sd_raw_t sd;
    if (!sd_raw_initialize(&sd, SD_PIN_CS)) {
        Serial.println("sd_raw_initialize failed");
        return;
    }

    uint32_t size = sd_raw_card_size(&sd);
    Serial.print("Card Size (x512): ");
    Serial.println(size);

    uint8_t block[512];

    if (!sd_raw_read_block(&sd, 0, block)) {
        Serial.println("sd_raw_read_block failed");
    }
    else {
        Serial.println("sd_raw_read_block done, writing...");

        for (uint16_t i = 0; i < 512; ++i) {
            block[i] = 256 - (i % 256);
        }

        if (!sd_raw_write_block(&sd, 0, block)) {
            Serial.println("sd_raw_read_block failed");
        }
        else {
            Serial.println("sd_raw_write_block done");
        }
    }
}

void loop() {

}
