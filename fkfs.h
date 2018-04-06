#ifndef FKFS_H_INCLUDED
#define FKFS_H_INCLUDED

#include <stdlib.h>
#include <stdint.h>

#include "sd_raw.h"

#define memzero(ptr, sz)          memset(ptr, 0, sz)

constexpr uint16_t FKFS_FILES_MAX = 4;
constexpr uint8_t FKFS_FILE_NAME_MAX = 12;

typedef struct fkfs_file_t {
    char name[FKFS_FILE_NAME_MAX];
    uint16_t version;
    uint32_t startBlock;
    uint16_t startOffset;
    uint32_t endBlock;
    uint16_t endOffset;
    uint32_t size;
} __attribute__((packed)) fkfs_file_t;

typedef struct fkfs_header_t {
    uint8_t version;
    uint32_t generation;
    uint32_t block;
    uint16_t offset;
    uint32_t time;
    fkfs_file_t files[FKFS_FILES_MAX];
    uint16_t crc;
} __attribute__((packed)) fkfs_header_t;

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

typedef struct fkfs_file_info_t {
    uint32_t size;
    uint8_t sync;
    uint8_t priority;
    uint16_t version;
    char name[FKFS_FILE_NAME_MAX];
} fkfs_file_info_t;

typedef struct fkfs_statistics_t {
    uint32_t blockReads;
    uint32_t blockWrites;
    uint32_t iterateCalls;
    uint32_t iterateTime;
    uint32_t writeTime;
    uint32_t readTime;
} fkfs_statistics_t;

void fkfs_statistics_zero(fkfs_statistics_t *fks);

typedef struct fkfs_t {
    uint8_t headerIndex;
    uint8_t cachedBlockDirty;
    uint32_t cachedBlockNumber;
    uint32_t numberOfBlocks;
    fkfs_header_t header;
    sd_raw_t sd;
    uint8_t buffer[SD_RAW_BLOCK_SIZE];
    fkfs_file_runtime_settings_t files[FKFS_FILES_MAX];
    fkfs_statistics_t statistics;
} fkfs_t;

typedef struct fkfs_iterator_token_t {
    uint8_t file;
    uint32_t block;
    uint16_t offset;
    uint32_t lastBlock;
    uint16_t lastOffset;
    uint32_t size;
} fkfs_iterator_token_t;

#define fkfs_token_empty    { 0, 0, 0, 0, 0, 0 }

typedef struct fkfs_iterator_config_t {
    uint32_t maxBlocks;
    uint32_t maxTime;
    uint8_t manualNext;
} fkfs_iterator_config_t;

typedef struct fkfs_file_iter_t {
    fkfs_iterator_token_t token;
    uint8_t *data;
    uint16_t size;
    uint32_t iterated;
} fkfs_file_iter_t;

static_assert(sizeof(fkfs_header_t) * 2 <= SD_RAW_BLOCK_SIZE, "Error: fkfs header too large for SD block.");

constexpr uint16_t FKFS_ENTRY_SIZE_MINUS_CRC = offsetof(fkfs_entry_t, crc);
constexpr uint16_t FKFS_HEADER_SIZE_MINUS_CRC = offsetof(fkfs_header_t, crc);
constexpr uint16_t FKFS_MAXIMUM_BLOCK_SIZE = SD_RAW_BLOCK_SIZE - sizeof(fkfs_entry_t);

uint8_t fkfs_configure_logging(size_t (*log_function_ptr)(const char *f, ...));

uint8_t fkfs_create(fkfs_t *fs);

uint8_t fkfs_touch(fkfs_t *fs, uint32_t time);

uint8_t fkfs_flush(fkfs_t *fs);

uint8_t fkfs_initialize_file(fkfs_t *fs, uint8_t fileNumber, uint8_t priority, uint8_t sync, const char *name);

uint8_t fkfs_initialize(fkfs_t *fs, bool wipe);

uint8_t fkfs_number_of_files(fkfs_t *fs);

uint8_t fkfs_get_file(fkfs_t *fs, uint8_t fileNumber, fkfs_file_info_t *info);

uint8_t fkfs_file_append(fkfs_t *fs, uint8_t fileNumber, uint16_t size, uint8_t *data);

uint8_t fkfs_file_truncate(fkfs_t *fs, uint8_t fileNumber);

uint8_t fkfs_file_truncate_at(fkfs_t *fs, fkfs_file_iter_t *iter);

uint8_t fkfs_file_truncate_all(fkfs_t *fs);

uint8_t fkfs_file_iterator_create(fkfs_t *fs, uint8_t fileNumber, fkfs_file_iter_t *iter);

uint8_t fkfs_file_iterator_reopen(fkfs_t *fs, fkfs_file_iter_t *iter, fkfs_iterator_token_t *token);

uint8_t fkfs_file_iterator_resume(fkfs_t *fs, fkfs_file_iter_t *iter, fkfs_iterator_token_t *token);

uint8_t fkfs_file_iterator_move_end(fkfs_t *fs, fkfs_file_iter_t *iter);

uint8_t fkfs_file_iterator_valid(fkfs_t *fs, fkfs_file_iter_t *iter);

uint8_t fkfs_file_iterate_move(fkfs_t *fs, bool checkBlock, fkfs_file_iter_t *iter);

uint8_t fkfs_file_iterate(fkfs_t *fs, fkfs_iterator_config_t *config, fkfs_file_iter_t *iter);

uint8_t fkfs_file_iterator_done(fkfs_t *fs, fkfs_file_iter_t *iter);

uint8_t fkfs_log_statistics(fkfs_t *fs);

#endif
