#include "fkfs.h"
#include <string.h>

#include <Arduino.h>
#define FKFS_LOG(msg)     Serial.println(msg)
#define FKFS_DEBUG

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

uint8_t fkfs_initialize_file(fkfs_t *fs, uint8_t fileNumber, uint8_t priority, uint8_t sync, const char *name) {
    fs->files[fileNumber].sync = sync;
    fs->files[fileNumber].priority = priority;

    fkfs_file_t *file = &fs->header.files[fileNumber];
    strncpy(file->name, name, sizeof(file->name));
    file->version = random(UINT16_MAX);
    file->startBlock = 1;

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
    fkfs_header_t *headers = (fkfs_header_t *)fs->buffer;

    fs->numberOfBlocks = sd_raw_card_size(&fs->sd);

    memzero(fs->buffer, sizeof(SD_RAW_BLOCK_SIZE));

    if (!sd_raw_read_block(&fs->sd, 0, (uint8_t *)fs->buffer)) {
        return false;
    }

    // If both checksums fail, then we're on a new card.
    // TODO: May want to make this configurable?
    if ((!fkfs_header_crc_valid(&headers[0]) &&
         !fkfs_header_crc_valid(&headers[1])) || wipe) {

        Serial.println("fkfs: initialize/wipe");

        // New filesystem... initialize a blank header and new versions of all files.
        for (uint8_t i = 0; i < FKFS_FILES_MAX; ++i) {
            fs->header.files[i].version = random(UINT16_MAX);
            Serial.print("file.version = ");
            Serial.println(fs->header.files[i].version);
        }
        fs->header.block = 1;

        // TODO: This is unnecessary?
        if (!fkfs_header_write(fs, fs->buffer)) {
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

        memcpy((void *)&fs->header, (void *)&headers[fs->headerIndex], sizeof(fkfs_header_t));
    }

    return true;
}

static uint16_t fkfs_block_crc(fkfs_t *fs, fkfs_file_t *file, fkfs_entry_t *entry, uint8_t *data) {
    uint16_t crc = file->version;

    crc = crc16_update(crc, (uint8_t *)entry, FKFS_ENTRY_SIZE_MINUS_CRC);
    crc = crc16_update(crc, (uint8_t *)data, entry->size);

    return crc;
}

static uint16_t fkfs_block_available_offset(fkfs_t *fs, fkfs_file_t *file, uint8_t priority, uint16_t required, uint8_t *buffer) {
    fkfs_entry_t *entry = (fkfs_entry_t *)buffer;
    uint16_t offset = fs->header.offset;

    #ifdef FKFS_DEBUG_VERBOSE
    Serial.print("fkfs: block_available_offset ");
    #endif

    do {
        #ifdef FKFS_DEBUG_VERBOSE
        Serial.print(offset);
        Serial.print(" ");
        #endif

        if (entry->file >= FKFS_FILES_MAX) {
            #ifdef FKFS_DEBUG_VERBOSE
            Serial.println("FILE");
            #endif
            return offset;
        }

        if (entry->size == 0 || entry->size >= SD_RAW_BLOCK_SIZE ||
            entry->available == 0 || entry->available >= SD_RAW_BLOCK_SIZE) {
            #ifdef FKFS_DEBUG_VERBOSE
            Serial.println("SIZE");
            #endif
            return offset;
        }

        uint8_t *data = ((uint8_t *)entry) + sizeof(fkfs_entry_t);
        uint16_t expected = fkfs_block_crc(fs, file, entry, data);
        if (entry->crc != expected) {
            #ifdef FKFS_DEBUG_VERBOSE
            Serial.println("CRC");
            #endif
            return offset;
        }

        // We have precedence over this entry?
        if (fs->files[entry->file].priority > priority) {
            if (entry->available >= required) {
                #ifdef FKFS_DEBUG_VERBOSE
                Serial.print(" [");
                Serial.print(fs->files[entry->file].priority);
                Serial.print(" > ");
                Serial.print(priority, HEX);
                Serial.print(" ");
                Serial.print(entry->available);
                Serial.print(" >= ");
                Serial.print(required);
                Serial.print("] PRI");
                Serial.println();
                #endif

                return offset;
            }
        }

        uint16_t occupied = sizeof(fkfs_entry_t) + entry->available;

        offset += occupied;
        entry = (fkfs_entry_t *)(buffer + offset);
    }
    while (offset + required < SD_RAW_BLOCK_SIZE);

    #ifdef FKFS_DEBUG_VERBOSE
    Serial.println("EOB");
    #endif

    return UINT16_MAX;
}

static uint8_t fkfs_fsync(fkfs_t *fs) {
    if (fs->cachedBlockDirty) {
        if (!sd_raw_write_block(&fs->sd, fs->header.block, (uint8_t *)fs->buffer)) {
            return false;
        }
        fs->cachedBlockNumber = UINT32_MAX;
        fs->cachedBlockDirty = false;
    }

    fs->header.generation++;
    fs->headerIndex = (fs->headerIndex + 1) % 2;

    if (!fkfs_header_write(fs, fs->buffer)) {
        return false;
    }

    fs->cachedBlockNumber = UINT32_MAX;
    fs->cachedBlockDirty = false;

    Serial.println("fkfs: sync!");

    return true;
}

static uint8_t fkfs_file_allocate_block(fkfs_t *fs, uint8_t fileNumber, uint16_t required, uint16_t size, fkfs_entry_t *entry) {
    fkfs_file_t *file = &fs->header.files[fileNumber];
    uint16_t newOffset = fs->header.offset;

    #ifdef FKFS_DEBUG_VERBOSE
    Serial.print("fkfs: file_allocate_block(");
    Serial.print(fileNumber);
    Serial.print(", ");
    Serial.print(required);
    Serial.print(")");
    Serial.println();
    #endif

    do {
        if (required + newOffset >= SD_RAW_BLOCK_SIZE) {
            if (fs->cachedBlockDirty) {
                if (!fkfs_fsync(fs)) {
                    return false;
                }
            }

            #ifdef FKFS_DEBUG_VERBOSE
            Serial.print("fkfs: new block #");
            Serial.print(fs->header.block + 1);
            Serial.print(" required=");
            Serial.print(required);
            Serial.print(" offset=");
            Serial.print(newOffset);
            Serial.println();
            #endif

            fs->header.block++;
            fs->header.offset = newOffset = 0;

            // Wrap around logic.
            if (fs->header.block == fs->numberOfBlocks - 2) {
                fs->header.block = 1;
            }
        }

        if (fs->cachedBlockNumber != fs->header.block) {
            if (!sd_raw_read_block(&fs->sd, fs->header.block, (uint8_t *)fs->buffer)) {
                return false;
            }
            fs->cachedBlockNumber = fs->header.block;
            fs->cachedBlockDirty = false;
        }

        newOffset = fkfs_block_available_offset(fs, file, fs->files[fileNumber].priority, required, fs->buffer);
        if (newOffset != UINT16_MAX) {
            fs->header.offset = newOffset;
            return true;
        }
        else {
            newOffset = SD_RAW_BLOCK_SIZE; // Force a move to the following block.
        }
    }
    while (true);

    return false;
}

uint8_t fkfs_file_append(fkfs_t *fs, uint8_t fileNumber, uint16_t size, uint8_t *data) {
    fkfs_entry_t entry = { 0 };
    fkfs_file_t *file = &fs->header.files[fileNumber];

    uint16_t required = sizeof(fkfs_entry_t) + size;
    if (size == 0 || required >= SD_RAW_BLOCK_SIZE) {
        return false;
    }

    if (!fkfs_file_allocate_block(fs, fileNumber, required, size, &entry)) {
        return false;
    }

    #ifdef FKFS_DEBUG
    Serial.print("fkfs: allocated f#");
    Serial.print(fileNumber);
    Serial.print(".");
    Serial.print(fs->files[fileNumber].priority, HEX);
    Serial.print(" ");
    Serial.print(fs->header.block);
    Serial.print("[");
    Serial.print(fs->header.offset);
    Serial.print("->");
    Serial.print(fs->header.offset + required);
    Serial.print("] ");
    Serial.print(SD_RAW_BLOCK_SIZE - (fs->header.offset + required));
    Serial.println();
    #endif

    entry.file = fileNumber;
    entry.size = size;
    entry.available = size;
    entry.crc = fkfs_block_crc(fs, file, &entry, data);

    // TODO: Maybe just cast the buffer to this?
    memcpy(((uint8_t *)fs->buffer) + fs->header.offset, (uint8_t *)&entry, sizeof(fkfs_entry_t));
    memcpy(((uint8_t *)fs->buffer) + fs->header.offset + sizeof(fkfs_entry_t), data, size);

    fs->cachedBlockDirty = true;
    fs->header.offset += required;

    if (fs->files[fileNumber].sync) {
        if (!fkfs_fsync(fs)) {
            return false;
        }
    }

    return true;
}

uint8_t fkfs_file_truncate(fkfs_t *fs, uint8_t fileNumber) {
    fkfs_file_t *file = &fs->header.files[fileNumber];

    file->version++;
    file->startBlock = fs->header.block;

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
