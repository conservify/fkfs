#include <stdio.h>
#include <Arduino.h>
#include <SPI.h>

#include <stdarg.h>
#include <stdio.h>

#include "sd_raw.h"
#include "fkfs.h"

#define FKFS_FILE_LOG                     0
#define FKFS_FILE_DATA                    1
#define FKFS_FILE_PRIORITY_LOWEST         255
#define FKFS_FILE_PRIORITY_HIGHEST        0

#define SD_PIN_CS1                        16
#define SD_PIN_CS2                        4

typedef struct fkfs_log_t {
    fkfs_t *fs;
    uint8_t file;
    size_t position;
    char buffer[FKFS_MAXIMUM_BLOCK_SIZE];
} fkfs_log_t;

uint8_t fkfs_log_initialize(fkfs_log_t *log, fkfs_t *fs, uint8_t file) {
    log->buffer[0] = 0;
    log->fs = fs;
    log->file = file;

    return true;
}

uint8_t fkfs_log_flush(fkfs_log_t *log) {
    if (!fkfs_file_append(log->fs, log->file, log->position, (uint8_t *)log->buffer)) {
        return false;
    }

    log->buffer[0] = 0;
    log->position = 0;

    return true;
}

uint8_t fkfs_log_append(fkfs_log_t *log, const char *str) {
    size_t required = strlen(str);
    const char *ptr = str;

    while (required > 0) {
        size_t available = FKFS_MAXIMUM_BLOCK_SIZE - log->position;
        size_t copy = required > available ? available : required;

        memcpy((uint8_t *)log->buffer + log->position, str, required);

        log->position += copy;
        required -= copy;
        str += copy;

        if (log->position >= FKFS_MAXIMUM_BLOCK_SIZE) {
            if (!fkfs_log_flush(log)) {
                return false;
            }
        }
    }

    return true;
}

uint8_t fkfs_log_printf(fkfs_log_t *log, const char *format, ...) {
    char incoming[FKFS_MAXIMUM_BLOCK_SIZE];
    va_list args;
    va_start(args, format);
    size_t length = vsnprintf(incoming, sizeof(incoming), format, args);
    va_end(args);

    return fkfs_log_append(log, incoming);
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
    Serial.println(seed);
    randomSeed(seed);

    fkfs_t fs = { 0 };
    fkfs_log_t log = { 0 };

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
