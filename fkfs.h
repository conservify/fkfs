#ifndef FKFS_H_INCLUDED
#define FKFS_H_INCLUDED

#include <stdlib.h>
#include <stdint.h>

#include "sd_raw.h"

#define memzero(ptr, sz)          memset(ptr, 0, sz)

const uint16_t FKFS_FILES_MAX = 6;

typedef struct fkfs_file_t {
    char name[12];
    uint16_t version;
    uint32_t startBlock;
} __attribute__((packed)) fkfs_file_t;

typedef struct fkfs_header_t {
    uint8_t version;
    uint32_t generation;
    uint32_t block;
    uint16_t offset;
    uint32_t time;
    fkfs_file_t files[FKFS_FILES_MAX];
    uint16_t crc;
} __attribute__((packed)) fkfs_header_t;;

typedef struct fkfs_entry_t {
    uint8_t file;
    uint16_t size;
    uint16_t available;
    uint16_t crc;
} __attribute__((packed)) fkfs_entry_t;

typedef struct fkfs_file_runtime_settings_t {
    uint8_t sync;
    uint8_t priority;
} fkfs_file_runtime_settings_t;

typedef struct fkfs_t {
    uint8_t headerIndex;
    uint8_t cachedBlockDirty;
    uint32_t cachedBlockNumber;
    uint32_t numberOfBlocks;
    fkfs_header_t header;
    sd_raw_t sd;
    uint8_t buffer[SD_RAW_BLOCK_SIZE];
    fkfs_file_runtime_settings_t files[FKFS_FILES_MAX];
} fkfs_t;

typedef struct fkfs_file_iter_t {
    uint8_t file;
    uint32_t block;
    uint16_t offset;
    uint8_t *data;
    uint16_t size;
};

const uint16_t FKFS_ENTRY_SIZE_MINUS_CRC = offsetof(fkfs_entry_t, crc);
const uint16_t FKFS_HEADER_SIZE_MINUS_CRC = offsetof(fkfs_header_t, crc);
const uint16_t FKFS_MAXIMUM_BLOCK_SIZE = SD_RAW_BLOCK_SIZE - sizeof(fkfs_entry_t);

uint8_t fkfs_create(fkfs_t *fs);

uint8_t fkfs_touch(fkfs_t *fs, uint32_t time);

uint8_t fkfs_flush(fkfs_t *fs);

uint8_t fkfs_initialize_file(fkfs_t *fs, uint8_t fileNumber, uint8_t priority, uint8_t sync, const char *name);

uint8_t fkfs_initialize(fkfs_t *fs, bool wipe);

uint8_t fkfs_file_append(fkfs_t *fs, uint8_t fileNumber, uint16_t size, uint8_t *data);

uint8_t fkfs_file_truncate(fkfs_t *fs, uint8_t fileNumber);

uint8_t fkfs_file_iterate(fkfs_t *fs, uint8_t fileNumber, fkfs_file_iter_t *iter);

uint8_t fkfs_log_statistics(fkfs_t *fs);

#endif
