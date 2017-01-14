#include <stdio.h>
#include <Arduino.h>
#include <SPI.h>

#include "sd_raw.h"
#include "fkfs.h"

#define FKFS_FILE_LOG                     0
#define FKFS_FILE_DATA                    1
#define FKFS_FILE_PRIORITY_LOWEST         255
#define FKFS_FILE_PRIORITY_HIGHEST        0

#define SD_PIN_CS1                        16
#define SD_PIN_CS2                        4

void setup() {
    Serial.begin(119200);

    // LoRa radio's CS.
    pinMode(8, OUTPUT);
    digitalWrite(8, HIGH);

    while (!Serial) {
        delay(100);
    }

    randomSeed(83473);

    uint32_t seed = 0;
    for (uint8_t i = 0; i < 10; ++i) {
        seed += analogRead(A0);
        seed += analogRead(A1);
        seed += analogRead(A3);
        seed += analogRead(A4);
    }
    Serial.println(seed);
    randomSeed(seed);

    fkfs_t fs;
    if (!fkfs_create(&fs)) {
        Serial.println("fkfs_create failed");
        return;
    }

    if (!sd_raw_initialize(&fs.sd, SD_PIN_CS1)) {
        if (!sd_raw_initialize(&fs.sd, SD_PIN_CS2)) {
            Serial.println("sd_raw_initialize failed");
            return;
        }
    }

    if (!fkfs_initialize_file(&fs, FKFS_FILE_LOG, FKFS_FILE_PRIORITY_LOWEST, false, "DEBUG.LOG")) {
        Serial.println("fkfs_initialize failed");
        return;
    }

    if (!fkfs_initialize_file(&fs, FKFS_FILE_DATA, FKFS_FILE_PRIORITY_HIGHEST, true, "DATA.BIN")) {
        Serial.println("fkfs_initialize failed");
        return;
    }

    if (!fkfs_initialize(&fs, true)) {
        Serial.println("fkfs_initialize failed");
        return;
    }

    fkfs_log_statistics(&fs);

    for (uint16_t i = 0; i < 396; ++i) {
        uint8_t buffer[256];
        memzero(buffer, sizeof(buffer));

        if (random(10) < 3) {
            memcpy(buffer, "DATA", 4);
            if (!fkfs_file_append(&fs, FKFS_FILE_DATA, random(240) + 5, buffer)) {
                Serial.println("fkfs_file_append DATA failed");
            }
        }
        else {
            memcpy(buffer, "LOGS", 4);
            if (!fkfs_file_append(&fs, FKFS_FILE_LOG, random(240) + 5, buffer)) {
                Serial.println("fkfs_file_append LOGS failed");
            }
        }
    }

    fkfs_file_iter_t iter = { 0 };
    while (fkfs_file_iterate(&fs, FKFS_FILE_LOG, &iter)) {
        Serial.print("fkfs: iter: ");
        Serial.print(iter.block);
        Serial.print(" ");
        Serial.print(iter.offset);
        Serial.print(" ");
        Serial.print((char *)iter.data);
        Serial.println();
    }
}


void loop() {

}
