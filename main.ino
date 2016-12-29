#include <Arduino.h>
#include <SPI.h>

#include "sd_raw.h"
#include "fkfs.h"

#define FKFS_FILE_LOG                     0
#define FKFS_FILE_DATA                    1
#define FKFS_FILE_PRIORITY_LOWEST         255
#define FKFS_FILE_PRIORITY_HIGHEST        0

#define SD_PIN_CS                         16

void setup() {
    Serial.begin(119200);

    // LoRa radio's CS.
    pinMode(8, OUTPUT);
    digitalWrite(8, HIGH);

    while (!Serial) {
        delay(100);
    }

    fkfs_t fs;
    if (!fkfs_create(&fs)) {
        Serial.println("fkfs_create failed");
        return;
    }

    if (!sd_raw_initialize(&fs.sd, SD_PIN_CS)) {
        Serial.println("sd_raw_initialize failed");
        return;
    }

    uint32_t size = sd_raw_card_size(&fs.sd);
    Serial.print("sd_raw: size (x512): ");
    Serial.println(size);

    if (!fkfs_initialize_file(&fs, FKFS_FILE_LOG, FKFS_FILE_PRIORITY_LOWEST, "DEBUG.LOG")) {
        Serial.println("fkfs_initialize failed");
        return;
    }

    if (!fkfs_initialize_file(&fs, FKFS_FILE_DATA, FKFS_FILE_PRIORITY_HIGHEST, "DATA.BIN")) {
        Serial.println("fkfs_initialize failed");
        return;
    }

    if (!fkfs_initialize(&fs, false)) {
        Serial.println("fkfs_initialize failed");
        return;
    }

    fkfs_log_statistics(&fs);

    uint8_t buffer[24];
    memset(buffer, 0, sizeof(buffer));

    for (uint8_t i = 0; i < 255; ++i) {
        if (i % 2 == 0) {
            memcpy(buffer, "World", 5);
            if (!fkfs_file_append(&fs, FKFS_FILE_DATA, sizeof(buffer), buffer)) {
                Serial.println("fkfs_file_append #1 failed");
                return;
            }
        }
        else {
            memcpy(buffer, "Hello", 5);
            if (!fkfs_file_append(&fs, FKFS_FILE_LOG, sizeof(buffer), buffer)) {
                Serial.println("fkfs_file_append #1 failed");
                return;
            }
        }

        fkfs_log_statistics(&fs);
    }
}

void loop() {

}
