#ifndef FKFS_H_INCLUDED
#define FKFS_H_INCLUDED

#include <stdlib.h>
#include <stdint.h>

#include "sd_raw.h"

#define memzero(ptr, sz)          memset(ptr, 0, sz)

const uint16_t FKFS_FILES_MAX = 4;
const uint16_t SD_RAW_BLOCK_SIZE = 512;

typedef struct fkfs_file_t {
    char name[12];
} __attribute__((packed)) fkfs_file_t;

typedef struct fkfs_header_t {
    uint8_t version;
    uint32_t generation;
    uint32_t block;
    uint16_t offset;
    fkfs_file_t files[FKFS_FILES_MAX];
    uint16_t crc;
} __attribute__((packed)) fkfs_header_t;;

typedef struct fkfs_entry_t {
    uint8_t file;
    uint16_t size;
    uint16_t crc;
} __attribute__((packed)) fkfs_entry_t;

typedef struct fkfs_file_runtime_settings_t {
    uint8_t sync;
    uint8_t priority;
} fkfs_file_runtime_settings_t;

typedef struct fkfs_t {
    uint8_t headerIndex;
    uint32_t cachedBlockNumber;
    fkfs_header_t header;
    sd_raw_t sd;
    uint8_t buffer[SD_RAW_BLOCK_SIZE];
    fkfs_file_runtime_settings_t files[FKFS_FILES_MAX];
} fkfs_t;

const uint16_t FKFS_HEADER_SIZE_MINUS_CRC = offsetof(fkfs_header_t, crc);

uint8_t fkfs_create(fkfs_t *fs);

uint8_t fkfs_initialize_file(fkfs_t *fs, uint8_t id, uint8_t priority, uint8_t sync, const char *name);

uint8_t fkfs_initialize(fkfs_t *fs, bool wipe);

uint8_t fkfs_file_append(fkfs_t *fs, uint8_t id, uint16_t size, uint8_t *data);

uint8_t fkfs_log_statistics(fkfs_t *fs);

#endif
