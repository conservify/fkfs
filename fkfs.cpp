#include "fkfs.h"
#include <string.h>

const uint16_t SD_RAW_BLOCK_SIZE = 512;

#include <Arduino.h>
#define FKFS_LOG(msg)     Serial.println(msg)
#define memzero(ptr, sz)  memset(ptr, 0, sz)

static uint32_t crc16_table[16] = {
    0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
    0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
};

static uint16_t crc16_update(uint16_t start, uint8_t *p, uint16_t n) {
    uint16_t crc = start;
    uint16_t r;

    while (n-- > 0) {
        /* compute checksum of lower four bits of *p */
        r = crc16_table[crc & 0xF];
        crc = (crc >> 4) & 0x0FFF;
        crc = crc ^ r ^ crc16_table[*p & 0xF];

        /* now compute checksum of upper four bits of *p */
        r = crc16_table[crc & 0xF];
        crc = (crc >> 4) & 0x0FFF;
        crc = crc ^ r ^ crc16_table[(*p >> 4) & 0xF];

        p++;
    }

    return crc;
}

#define FKFS_HEADER_SIZE_MINUS_CRC offsetof(fkfs_header_t, crc)

static uint8_t fkfs_header_crc_valid(fkfs_header_t *header) {
    uint16_t actual = crc16_update(0, (uint8_t *)header, FKFS_HEADER_SIZE_MINUS_CRC);
    return header->crc == actual;
}

static uint8_t fkfs_header_crc_update(fkfs_header_t *header) {
    uint16_t actual = crc16_update(0, (uint8_t *)header, FKFS_HEADER_SIZE_MINUS_CRC);
    header->crc = actual;
    return actual;
}

uint8_t fkfs_create(fkfs_t *fs) {
    memzero(fs, sizeof(fkfs_t));

    return true;
}

uint8_t fkfs_initialize_file(fkfs_t *fs, uint8_t id, uint8_t priority, const char *name) {
    fs->header.files[id].priority = priority;
    strncpy(fs->header.files[id].name, name, sizeof(fs->header.files[id].name));

    return true;
}

static uint8_t fkfs_header_write(fkfs_t *fs, uint8_t *temp) {
    fkfs_header_t *headers = (fkfs_header_t *)temp;

    fkfs_header_crc_update(&fs->header);

    memcpy((void *)&headers[fs->headerIndex], (void *)&fs->header, sizeof(fkfs_header_t));

    if (!sd_raw_write_block(&fs->sd, 0, (uint8_t *)temp)) {
        return false;
    }

    return true;
}

uint8_t fkfs_initialize(fkfs_t *fs, bool wipe) {
    uint8_t buffer[SD_RAW_BLOCK_SIZE];
    fkfs_header_t *headers = (fkfs_header_t *)buffer;

    memzero(buffer, sizeof(SD_RAW_BLOCK_SIZE));

    if (!sd_raw_read_block(&fs->sd, 0, (uint8_t *)buffer)) {
        return false;
    }

    // If both checksums fail, then we're on a new card.
    // TODO: May want to make this configurable?
    if ((!fkfs_header_crc_valid(&headers[0]) &&
         !fkfs_header_crc_valid(&headers[1])) ||
         wipe) {

        FKFS_LOG("fkfs: new filesystem");

        // New filesystem... initialize a blank header.
        fs->header.block = 1;
        if (!fkfs_header_write(fs, buffer)) {
            return false;
        }
    }
    else {
        if (!fkfs_header_crc_valid(&headers[1])) {
            fs->headerIndex = 0;
        }
        else if (!fkfs_header_crc_valid(&headers[0])) {
            fs->headerIndex = 1;
        }
        else if (headers[0].generation > headers[1].generation) {
            fs->headerIndex = 0;
        }
        else {
            fs->headerIndex = 1;
        }

        Serial.print("fkfs: using header ");
        Serial.println(fs->headerIndex);
        memcpy((void *)&fs->header, (void *)&headers[fs->headerIndex], sizeof(fkfs_header_t));
    }

    return true;
}

uint8_t fkfs_file_append(fkfs_t *fs, uint8_t file, uint16_t size, uint8_t *data) {
    uint8_t buffer[SD_RAW_BLOCK_SIZE];
    uint16_t required = sizeof(fkfs_entry_t) + size;
    fkfs_entry_t entry = { 0 };

    if (required > SD_RAW_BLOCK_SIZE) {
        return false;
    }

    if (required + fs->header.offset >= SD_RAW_BLOCK_SIZE) {
        fs->header.block++;
        fs->header.offset = 0;
    }

    if (!sd_raw_read_block(&fs->sd, fs->header.block, (uint8_t *)buffer)) {
        return false;
    }

    entry.file = file;
    entry.size = size;
    entry.crc = crc16_update(0, data, size);

    // TODO: Maybe just cast the buffer to this?
    memcpy(((uint8_t *)buffer) + fs->header.offset, (uint8_t *)&entry, sizeof(fkfs_entry_t));

    memcpy(((uint8_t *)buffer) + fs->header.offset + sizeof(fkfs_entry_t), data, size);

    if (!sd_raw_write_block(&fs->sd, fs->header.block, (uint8_t *)buffer)) {
        return false;
    }

    fs->header.offset += required;
    fs->header.generation++;
    fs->headerIndex = (fs->headerIndex + 1) % 2;

    if (!fkfs_header_write(fs, buffer)) {
        return false;
    }

    return true;
}

uint8_t fkfs_log_statistics(fkfs_t *fs) {
    Serial.print("fkfs: index=");
    Serial.print(fs->headerIndex);
    Serial.print(" gen=");
    Serial.print(fs->header.generation);
    Serial.print(" block=");
    Serial.print(fs->header.block);
    Serial.print(" offset=");
    Serial.print(fs->header.offset);
    Serial.println();

    return true;
}
