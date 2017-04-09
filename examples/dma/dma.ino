#include <stdio.h>
#include <Arduino.h>
#include <SPI.h>

#include "sd_raw.h"
#include "sd_raw_internal.h"

#define FKFS_FILE_LOG                     0
#define FKFS_FILE_DATA                    1
#define FKFS_FILE_PRIORITY_LOWEST         255
#define FKFS_FILE_PRIORITY_HIGHEST        0

#define SD_PIN_CS1                        16
#define SD_PIN_CS2                        4

void log_buffer(uint8_t *buffer) {
    for (uint32_t i = 0; i < DATA_LENGTH; i++) {
        Serial.print(buffer[i], HEX); Serial.print(' ');
        if ((i % 16) == 15)
            Serial.println();
    }
}
void setup() {
    Serial.begin(119200);

    // LoRa radio's CS.
    pinMode(8, OUTPUT);
    digitalWrite(8, HIGH);

    while (!Serial) {
        delay(100);
    }

    sd_raw_t sd;

    if (!sd_raw_initialize(&sd, SD_PIN_CS1)) {
        if (!sd_raw_initialize(&sd, SD_PIN_CS2)) {
            Serial.println("sd_raw_initialize failed");
            while (true) {
                delay(100);
            }
        }
    }

    uint8_t status;
    #define DATA_LENGTH 512
    uint8_t source_memory[DATA_LENGTH];
    uint8_t destination_memory[DATA_LENGTH];

    for (uint16_t i = 0; i < DATA_LENGTH; ++i) {
        source_memory[i] = 0;
    }

    if (!sd_raw_write_block(&sd, 0, source_memory)) {
        Serial.print("sd_raw_write_block block failed: ");
        Serial.println(status);
        while (true) {
            delay(100);
        }
    }

    sd_raw_dma_t sd_dma = { 0 };

    sd_raw_dma_initialize(&sd_dma, &sd, source_memory, destination_memory, 512);

    Serial.println("dma_write_block");

    for (uint32_t i = 0; i < DATA_LENGTH; i++) {
        source_memory[i] = i;
    }

    status = sd_raw_dma_write_block(&sd_dma, 0);
    status = true;
    if (!status) {
        Serial.print("ERROR: ");
        Serial.println(status);
    }
    else {
        Serial.println("dma_read_block");

        status = sd_raw_dma_read_block(&sd_dma, 0);
        if (!status) {
            Serial.print("ERROR: ");
            Serial.println(status);
        }
        else {
            log_buffer(destination_memory);
        }
    }

    Serial.println("done");

    while (true) {
        delay(100);
    }
}


void loop() {

}
