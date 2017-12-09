#include <stdio.h>
#include <Arduino.h>
#include <SPI.h>

#include "sd_raw.h"
#include "fkfs.h"
#include "fkfs_log.h"

#define FKFS_FILE_LOG                     0
#define FKFS_FILE_DATA                    1
#define FKFS_FILE_PRIORITY_LOWEST         255
#define FKFS_FILE_PRIORITY_HIGHEST        0

#define SD_PIN_CS1                        16
#define SD_PIN_CS2                        4

typedef struct fkfs_single_record_t {
    fkfs_t *fs;
    uint8_t file;
} fkfs_single_record_t;

uint8_t fkfs_single_record_initialize(fkfs_single_record_t *sr, fkfs_t *fs, uint8_t file) {
    sr->fs = fs;
    sr->file = file;

    return true;
}

uint8_t fkfs_single_record_read(fkfs_single_record_t *sr, uint8_t *ptr, uint16_t size) {
    fkfs_file_iter_t iter = { 0 };

    if (fkfs_file_iterate(sr->fs, FKFS_FILE_LOG, &iter, nullptr)) {
        if (iter.size != size) {
            return false;
        }

        memcpy(ptr, iter.data, size);

        // TODO: Should actually never be more than one block.
        return true;
    }

    return false;
}

uint8_t fkfs_single_record_write(fkfs_single_record_t *sr, uint8_t *ptr, uint16_t size) {
    if (!fkfs_file_truncate(sr->fs, sr->file)) {
        return false;
    }

    return fkfs_file_append(sr->fs, sr->file, size, ptr);
}

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
    randomSeed(seed);

    fkfs_t fs = { 0 };
    fkfs_log_t log = { 0 };

    Serial.print("Header Size: ");
    Serial.println(sizeof(fkfs_header_t));
    Serial.print("Header Size (minus files): ");
    Serial.println(sizeof(fkfs_header_t) - sizeof(fkfs_file_t) * FKFS_FILES_MAX);
    Serial.print("File Header Size: ");
    Serial.println(sizeof(fkfs_file_t));
    Serial.print("Entry Header Size: ");
    Serial.println(sizeof(fkfs_entry_t));

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

    if (!fkfs_log_initialize(&log, &fs, FKFS_FILE_LOG)) {
        Serial.println("fkfs_log_initialize failed");
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
            if (!fkfs_log_printf(&log, "Hello, world")) {
                Serial.println("fkfs_log_append failed");
            }
        }
    }

    fkfs_file_iter_t iter = { 0 };
    while (fkfs_file_iterate(&fs, FKFS_FILE_LOG, &iter, nullptr)) {
        Serial.print("fkfs: iter: ");
        Serial.print(iter.token.block);
        Serial.print(" ");
        Serial.print(iter.token.offset);
        Serial.print(" ");
        Serial.print((char *)iter.data);
        Serial.println();
    }
}


void loop() {

}
