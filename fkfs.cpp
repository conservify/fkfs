#include <Arduino.h>

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "fkfs.h"

size_t fkfs_printf(const char *f, ...) {
    char buffer[256];
    va_list args;
    va_start(args, f);
    vsnprintf(buffer, sizeof(buffer), f, args);
    Serial.print(buffer);
    va_end(args);
    return 0;
}

static size_t (*fkfs_log_function_ptr)(const char *f, ...) = fkfs_printf;

#define FKFS_FIRST_BLOCK           8000
#define FKFS_SEEK_BLOCKS_MAX       5

// This is for testing wrap around.
#define FKFS_TESTING_LAST_BLOCK    UINT32_MAX

#ifdef FKFS_LOGGING
#define fkfs_log(f, ...)           fkfs_log_function_ptr(f, ##__VA_ARGS__)
#else
#define fkfs_log(f, ...)
#endif

#ifdef FKFS_LOGGING_VERBOSE
#define fkfs_log_verbose(f, ...)   fkfs_log(f, ##__VA_ARGS__)
#else
#define fkfs_log_verbose(f, ...)
#endif

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
    uint16_t actual = crc16_update(31337, (uint8_t *)header, FKFS_HEADER_SIZE_MINUS_CRC);
    return header->crc == actual;
}

static uint8_t fkfs_header_crc_update(fkfs_header_t *header) {
    uint16_t actual = crc16_update(31337, (uint8_t *)header, FKFS_HEADER_SIZE_MINUS_CRC);
    header->crc = actual;
    return actual;
}

void fkfs_statistics_zero(fkfs_statistics_t *fks) {
    fks->blockReads = 0;
    fks->blockWrites = 0;
    fks->iterateCalls = 0;
    fks->iterateTime = 0;
    fks->writeTime = 0;
    fks->readTime = 0;
}

uint8_t fkfs_configure_logging(size_t (*log_function_ptr)(const char *f, ...)) {
    fkfs_log_function_ptr = log_function_ptr;
    return true;
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
    file->startBlock = FKFS_FIRST_BLOCK;
    file->startOffset = 0;
    file->endOffset = 0;

    return true;
}

uint8_t fkfs_number_of_files(fkfs_t *fs) {
    for (uint8_t counter = 0; counter < FKFS_FILES_MAX; ++counter) {
        fkfs_file_t *file = &fs->header.files[counter];
        if (file->name[0] == 0) {
            return counter;
        }
    }
    return 0;
}

uint8_t fkfs_get_file(fkfs_t *fs, uint8_t fileNumber, fkfs_file_info_t *info) {
    info->sync = fs->files[fileNumber].sync;
    info->priority = fs->files[fileNumber].priority;

    fkfs_file_t *file = &fs->header.files[fileNumber];
    strncpy(info->name, file->name, sizeof(info->name));
    info->version = file->version;;
    info->size = file->size;

    return true;
}

static uint8_t fkfs_read_block(fkfs_t *fs, uint32_t block, uint8_t *buffer) {
    fs->statistics.blockReads++;

    fkfs_log("fkfs: read block %d (%x)", block, buffer);

    auto started = millis();
    auto status = true;
    if (!sd_raw_read_block(&fs->sd, block, (uint8_t *)buffer)) {
        status = false;
    }

    fs->statistics.readTime += millis() - started;

    return status;
}

static uint8_t fkfs_write_block(fkfs_t *fs, uint32_t block, uint8_t *buffer) {
    fs->statistics.blockWrites++;

    auto started = millis();
    auto status = true;
    if (!sd_raw_write_block(&fs->sd, block, (uint8_t *)buffer)) {
        status = false;
    }

    fs->statistics.writeTime += millis() - started;

    return status;
}

static uint8_t fkfs_header_write(fkfs_t *fs, bool wipe) {
    uint8_t buffer[SD_RAW_BLOCK_SIZE] = { 0 };

    if (!wipe) {
        if (!fkfs_read_block(fs, 0, (uint8_t *)buffer)) {
            return false;
        }
    }

    fkfs_header_t *headers = (fkfs_header_t *)buffer;

    fkfs_header_crc_update(&fs->header);

    memcpy((void *)&headers[fs->headerIndex], (void *)&fs->header, sizeof(fkfs_header_t));

    if (!fkfs_write_block(fs, 0, (uint8_t *)buffer)) {
        return false;
    }

    return true;
}

static uint8_t fkfs_block_ensure(fkfs_t *fs, uint32_t block) {
    if (fs->cachedBlockDirty) {
    }

    if (fs->cachedBlockNumber != block) {
        if (!fkfs_read_block(fs, block, (uint8_t *)fs->buffer)) {
            return false;
        }
        fs->cachedBlockNumber = block;
        fs->cachedBlockDirty = false;
    }
    return true;
}

uint8_t fkfs_initialize(fkfs_t *fs, bool wipe) {
    fs->numberOfBlocks = sd_raw_card_size(&fs->sd);

    memzero(fs->buffer, sizeof(SD_RAW_BLOCK_SIZE));

    fkfs_statistics_zero(&fs->statistics);

    if (!fkfs_read_block(fs, 0, (uint8_t *)fs->buffer)) {
        return false;
    }

    fkfs_header_t *headers = (fkfs_header_t *)fs->buffer;

    // If both checksums fail, then we're on a new card.
    // TODO: May want to make this configurable?
    if (wipe || (!fkfs_header_crc_valid(&headers[0]) && !fkfs_header_crc_valid(&headers[1]))) {
        fkfs_log("fkfs: initialize/wipe");

        fs->header.block = FKFS_FIRST_BLOCK;
        fs->header.generation = 0;

        // New filesystem... initialize a blank header and new versions of all files.
        for (uint8_t i = 0; i < FKFS_FILES_MAX; ++i) {
            fs->header.files[i].version = random(UINT16_MAX);
            fs->header.files[i].size = 0;
            fs->header.files[i].startBlock = fs->header.block;
            fs->header.files[i].startOffset = 0;
            fs->header.files[i].endBlock = fs->header.block;
            fs->header.files[i].endOffset = 0;

            fkfs_log("file[%d] sync=%d pri=%d sb=%d eb=%d version=%d size=%d '%s'", i,
                     fs->files[i].sync,
                     fs->files[i].priority,
                     fs->header.files[i].startBlock,
                     fs->header.files[i].endBlock,
                     fs->header.files[i].version,
                     fs->header.files[i].size,
                     fs->header.files[i].name);
        }

        memcpy((void *)&headers[fs->headerIndex], (void *)&fs->header, sizeof(fkfs_header_t));

        if (!fkfs_header_write(fs, true)) {
            return false;
        }

        fs->header.generation = 1;
        fs->headerIndex = 1;

        memcpy((void *)&headers[fs->headerIndex], (void *)&fs->header, sizeof(fkfs_header_t));

        if (!fkfs_header_write(fs, false)) {
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

        for (auto i = 0; i < FKFS_FILES_MAX; ++i) {
            strncpy(headers[fs->headerIndex].files[i].name, fs->header.files[i].name, sizeof(headers[fs->headerIndex].files[i].name));
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

#define FKFS_OFFSET_SEARCH_STATUS_GOOD      0
#define FKFS_OFFSET_SEARCH_STATUS_SIZE      1
#define FKFS_OFFSET_SEARCH_STATUS_CRC       2
#define FKFS_OFFSET_SEARCH_STATUS_PRIORITY  3
#define FKFS_OFFSET_SEARCH_STATUS_EOB       4

static const char *block_check_str(uint8_t check) {
    switch (check) {
    case FKFS_OFFSET_SEARCH_STATUS_GOOD: return "good";
    case FKFS_OFFSET_SEARCH_STATUS_SIZE: return "size";
    case FKFS_OFFSET_SEARCH_STATUS_CRC: return "crc";
    case FKFS_OFFSET_SEARCH_STATUS_PRIORITY: return "priority";
    case FKFS_OFFSET_SEARCH_STATUS_EOB: return "eob";
    default:
        return "unknown";
    }
}

typedef struct fkfs_offset_search_t {
    uint16_t offset;
    uint8_t status;
} fkfs_offset_search_t;

static uint8_t fkfs_block_check(fkfs_t *fs, uint8_t *ptr) {
    fkfs_entry_t *entry = (fkfs_entry_t *)ptr;

    if (entry->file >= FKFS_FILES_MAX) {
        return FKFS_OFFSET_SEARCH_STATUS_SIZE;
    }

    // TODO: This should really compare to the header adjusted lengths....
    if (entry->size == 0 || entry->size >= SD_RAW_BLOCK_SIZE ||
        entry->available == 0 || entry->available >= SD_RAW_BLOCK_SIZE) {
        return FKFS_OFFSET_SEARCH_STATUS_SIZE;
    }

    fkfs_file_t *blockFile = &fs->header.files[entry->file];
    uint8_t *data = ptr + sizeof(fkfs_entry_t);
    uint16_t expected = fkfs_block_crc(fs, blockFile, entry, data);
    if (entry->crc != expected) {
        return FKFS_OFFSET_SEARCH_STATUS_CRC;
    }

    return FKFS_OFFSET_SEARCH_STATUS_GOOD;
}

static uint8_t fkfs_block_available_offset(fkfs_t *fs, fkfs_file_t *file, uint8_t priority, uint16_t required, uint8_t *buffer, fkfs_offset_search_t *search) {
    uint8_t *iter = buffer + search->offset;
    fkfs_entry_t *entry = (fkfs_entry_t *)iter;
#ifdef FKFS_LOGGING_VERBOSE
    uint16_t initialOffset = search->offset;
#endif

    fkfs_log_verbose("fkfs: block_available_offset(%d, %d) ", search->offset, required);

    do {
        search->status = fkfs_block_check(fs, iter);

        switch (search->status) {
        case FKFS_OFFSET_SEARCH_STATUS_SIZE:
            fkfs_log_verbose("fkfs: invalid size at %d", search->offset);
            return true;
        case FKFS_OFFSET_SEARCH_STATUS_CRC:
            fkfs_log_verbose("fkfs: invalid crc at %d", search->offset);
            return true;
        case FKFS_OFFSET_SEARCH_STATUS_GOOD:
            break;
        }

        // We have precedence over this entry?
        uint8_t blockPriority = fs->files[entry->file].priority;
        if (blockPriority > priority) {
            if (entry->available >= required) {
                search->status = FKFS_OFFSET_SEARCH_STATUS_PRIORITY;
                fkfs_log_verbose(" [%d > %d][%d >= %d] PRI",
                                 blockPriority, priority,
                                 entry->available, required);
                return true;
            }
        }

        uint16_t occupied = sizeof(fkfs_entry_t) + entry->available;

        search->offset += occupied;
        iter = buffer + search->offset;
        entry = (fkfs_entry_t *)iter;
    }
    while (search->offset + required < SD_RAW_BLOCK_SIZE);

    search->status = FKFS_OFFSET_SEARCH_STATUS_EOB;

#ifdef FKFS_LOGGING_VERBOSE
    fkfs_log_verbose("EOB: block=%d required=%d offset=%d initialOffset=%d version=%d", fs->header.block, required, search->offset, initialOffset, file->version);
    if (initialOffset == 0) {
        fkfs_entry_t *entry = (fkfs_entry_t *)buffer + initialOffset;
        fkfs_file_t *blockFile = &fs->header.files[entry->file];
        uint8_t *data = buffer + sizeof(fkfs_entry_t);
        uint16_t expected = fkfs_block_crc(fs, blockFile, entry, data);
        fkfs_log_verbose("ENTRY: file(%d) size(%d) version(%d) crc(%d vs %d)", entry->file, entry->size, blockFile->version, entry->crc, expected);
    }
#endif

    return false;
}

static uint8_t fkfs_fsync(fkfs_t *fs) {
    if (fs->cachedBlockDirty) {
        if (!fkfs_write_block(fs, fs->header.block, (uint8_t *)fs->buffer)) {
            return false;
        }
        fs->cachedBlockNumber = UINT32_MAX;
        fs->cachedBlockDirty = false;
    }
    else {
        // No reason to write anything if there's nothing dirty.
        fkfs_log_verbose("fkfs: sync (ignored)");
        return true;
    }

    fs->header.generation++;
    fs->headerIndex = (fs->headerIndex + 1) % 2;

    if (!fkfs_header_write(fs, false)) {
        return false;
    }

    fs->cachedBlockNumber = UINT32_MAX;
    fs->cachedBlockDirty = false;

    fkfs_log_verbose("fkfs: sync!");

    return true;
}

uint8_t fkfs_touch(fkfs_t *fs, uint32_t time) {
    fs->header.time = time;

    if (!fkfs_header_write(fs, false)) {
        return false;
    }

    return true;
}

uint8_t fkfs_flush(fkfs_t *fs) {
    if (!fkfs_touch(fs, millis())) {
        return false;
    }

    if (!fkfs_fsync(fs)) {
        return false;
    }

    return true;
}

static uint8_t fkfs_file_allocate_block(fkfs_t *fs, uint8_t fileNumber, uint16_t required, uint16_t size, fkfs_entry_t *entry) {
    fkfs_file_t *file = &fs->header.files[fileNumber];
    uint16_t newOffset = fs->header.offset;
    uint16_t visitedBlocks = 0;

    fkfs_log_verbose("fkfs: file_allocate_block(%d, %d) (block=%d, offset=%d)", fileNumber, required, fs->header.block, newOffset);

    do {
        // If we can't fit in the remainder of this block, we gotta move on.
        if (required + newOffset > SD_RAW_BLOCK_SIZE) {
            // Flush any cached block before we move onto a new block.
            if (!fkfs_fsync(fs)) {
                return false;
            }

            // Next block.
            fs->header.block++;
            fs->header.offset = newOffset = 0;
            visitedBlocks++;

            fkfs_log_verbose("fkfs: file_allocate_block(%d, %d) (new block %d)", fileNumber, required, fs->header.block);

            // Wrap around logic, back to the beginning of the SD. It will now
            // be important to look at priority and for old files.
            if (fs->header.block == fs->numberOfBlocks - 2 || fs->header.block == FKFS_TESTING_LAST_BLOCK) {
                fs->header.block = FKFS_FIRST_BLOCK;
                fkfs_log_verbose("fkfs: file_allocate_block(%d, %d) (wrap around %d)", fileNumber, required, fs->header.block);
            }
        }

        // If this isn't the block we have cached then read the block, this is
        // for when we've moved to a new block or were just opened.
        if (fs->cachedBlockNumber != fs->header.block) {
            if (!fkfs_read_block(fs, fs->header.block, (uint8_t *)fs->buffer)) {
                return false;
            }
            fs->cachedBlockNumber = fs->header.block;
            fs->cachedBlockDirty = false;
        }

        // See if we can find a place for ourselves in the block. This involves
        // looping over the existing chain of blocks.
        fkfs_offset_search_t search = { 0 };
        search.offset = newOffset;
        if (fkfs_block_available_offset(fs, file, fs->files[fileNumber].priority, required, fs->buffer, &search)) {
            // We found a place to store the data.
            fs->header.offset = search.offset;
            return true;
        }
        else {
            newOffset = SD_RAW_BLOCK_SIZE; // Force a move to the following block.
        }
    }
    while (visitedBlocks < FKFS_SEEK_BLOCKS_MAX);

    return false;
}

uint8_t fkfs_file_append(fkfs_t *fs, uint8_t fileNumber, uint16_t size, uint8_t *data) {
    fkfs_entry_t entry = { 0 };
    fkfs_file_t *file = &fs->header.files[fileNumber];

    // Just fail if we'll never be able to store this block. The upper layers
    // should never allow this.
    uint16_t required = sizeof(fkfs_entry_t) + size;
    if (size == 0 || required > SD_RAW_BLOCK_SIZE) {
        return false;
    }

    fkfs_log_verbose("fkfs: allocating f#%d %-3d.%-5d %3d[required = %d (+%d) = %d]",
                     fileNumber, fs->files[fileNumber].priority, file->version,
                     fs->header.block, size, sizeof(fkfs_entry_t), required);

    if (!fkfs_file_allocate_block(fs, fileNumber, required, size, &entry)) {
        return false;
    }

    fkfs_log("fkfs: allocated  f#%d %3d[%-3d -> %-3d] [%3d / %3d] %d",
             fileNumber, fs->header.block,
             fs->header.offset, fs->header.offset + required,
             size, required,
             SD_RAW_BLOCK_SIZE - (fs->header.offset + required));

    entry.file = fileNumber;
    entry.size = size;
    entry.available = size;
    entry.crc = fkfs_block_crc(fs, file, &entry, data);

    // TODO: Maybe just cast the buffer to this?
    memcpy(((uint8_t *)fs->buffer) + fs->header.offset, (uint8_t *)&entry, sizeof(fkfs_entry_t));
    memcpy(((uint8_t *)fs->buffer) + fs->header.offset + sizeof(fkfs_entry_t), data, size);

    fs->cachedBlockDirty = true;
    fs->header.offset += required;
    fs->header.files[fileNumber].endBlock = fs->header.block;
    fs->header.files[fileNumber].endOffset = fs->header.offset;
    fs->header.files[fileNumber].size += size;

    // If this file is configured to be fsync'd after every write that go ahead
    // and do that here. Otherwise this will happen later, either manually or
    // when we need to seek to a new block.
    if (fs->files[fileNumber].sync) {
        if (!fkfs_fsync(fs)) {
            return false;
        }

        fkfs_log_verbose("fkfs: done, synced");
    } else {
        fkfs_log_verbose("fkfs: done, file is no-sync");
    }

    return true;
}

uint8_t fkfs_file_truncate(fkfs_t *fs, uint8_t fileNumber) {
    fkfs_file_t *file = &fs->header.files[fileNumber];

    fkfs_log("fkfs: truncate %d", fileNumber);

    // Bump versions so CRC checks fail on previous blocks and store the new
    // starting block for the file.
    file->version++;
    file->startBlock = fs->header.block;
    file->endBlock = file->startBlock;
    file->startOffset = 0;
    file->endOffset = 0;
    file->size = 0;

    return true;
}

static uint8_t calculate_file_size(fkfs_t *fs, uint8_t fileNumber) {
    fkfs_file_t *file = &fs->header.files[fileNumber];
    file->size = 0;

    fkfs_file_iter_t iter;
    fkfs_file_iterator_create(fs, fileNumber, &iter);

    fkfs_iterator_config_t config = {
        .maxBlocks = 10,
        .maxTime = 0,
    };
    while (fkfs_file_iterate(fs, &config, &iter)) {
        file->size += iter.size;
    }
    return true;
}

uint8_t fkfs_file_truncate_at(fkfs_t *fs, fkfs_file_iter_t *iter) {
    fkfs_file_t *file = &fs->header.files[iter->token.file];

    file->startBlock = iter->token.lastBlock;

    return calculate_file_size(fs, iter->token.file);
}

uint8_t fkfs_file_truncate_all(fkfs_t *fs) {
    for (auto i = 0; i < FKFS_FILES_MAX; ++i) {
        fkfs_file_truncate(fs, i);
    }
    return true;
}

uint8_t fkfs_file_iterator_create(fkfs_t *fs, uint8_t fileNumber, fkfs_file_iter_t *iter) {
    fkfs_file_t *file = &fs->header.files[fileNumber];
    iter->token.file = fileNumber;
    iter->token.block = file->startBlock;
    iter->token.offset = 0;
    iter->token.lastBlock = file->endBlock;
    iter->token.lastOffset = file->endOffset;
    iter->token.size = file->size;

    fkfs_log("fkfs: iter create %d", fileNumber);

    return true;
}

uint8_t fkfs_file_iterator_reopen(fkfs_t *fs, fkfs_file_iter_t *iter, fkfs_iterator_token_t *token) {
    fkfs_file_t *file = &fs->header.files[token->file];
    if (file->size < token->size) {
        fkfs_file_iterator_create(fs, token->file, iter);
        return true;
    }
    iter->token.file = token->file;
    iter->token.block = token->block;
    iter->token.offset = token->offset;
    iter->token.lastBlock = file->endBlock;
    iter->token.lastOffset = file->endOffset;
    iter->token.size = file->size;

    fkfs_log("fkfs: iter reopen %d", token->file);

    return true;
}

uint8_t fkfs_file_iterator_resume(fkfs_t *fs, fkfs_file_iter_t *iter, fkfs_iterator_token_t *token) {
    fkfs_file_t *file = &fs->header.files[token->file];
    if (file->size < token->size) {
        fkfs_file_iterator_create(fs, token->file, iter);
        return true;
    }
    iter->token.file = token->file;
    iter->token.block = token->block;
    iter->token.offset = token->offset;
    iter->token.lastBlock = token->lastBlock;
    iter->token.lastOffset = token->lastOffset;
    iter->token.size = token->size;

    fkfs_log("fkfs: iter resume %d", token->file);

    return true;
}

uint8_t fkfs_file_iterator_done(fkfs_t *fs, fkfs_file_iter_t *iter) {
    return iter->token.block > iter->token.lastBlock || (iter->token.block == iter->token.lastBlock && iter->token.offset >= iter->token.lastOffset);
}

uint8_t fkfs_file_iterator_valid(fkfs_t *fs, fkfs_file_iter_t *iter) {
    return iter->token.block > 0 && iter->token.block <= fs->header.block && (iter->token.block < iter->token.lastBlock || (iter->token.block == iter->token.lastBlock && iter->token.offset <= iter->token.lastOffset));
}

uint8_t fkfs_file_iterator_move_end(fkfs_t *fs, fkfs_file_iter_t *iter) {
    iter->token.block = iter->token.lastBlock;
    iter->token.offset = iter->token.lastOffset;
    return true;
}

uint8_t fkfs_file_iterate_move(fkfs_t *fs, bool checkBlock, fkfs_file_iter_t *iter) {
    auto ptr = fs->buffer + iter->token.offset;
    if (checkBlock) {
        auto check = fkfs_block_check(fs, ptr);
        if (check != FKFS_OFFSET_SEARCH_STATUS_CRC && check != FKFS_OFFSET_SEARCH_STATUS_GOOD) {
            return false;
        }
    }

    auto entry = (fkfs_entry_t *)ptr;
    iter->token.offset += entry->available + sizeof(fkfs_entry_t);

    return true;
}

uint8_t fkfs_file_iterate(fkfs_t *fs, fkfs_iterator_config_t *config, fkfs_file_iter_t *iter) {
    fs->statistics.iterateCalls++;

    // Check for a valid token.
    if (!fkfs_file_iterator_valid(fs, iter)) {
        fkfs_log("fkfs: scanning: iterator invalid");
        return false;
    }

    if (fkfs_file_iterator_done(fs, iter)) {
        fkfs_log("fkfs: scanning: iterator done (%d)", iter->token.block);
        return false;
    }

    fkfs_log_verbose("fkfs: scanning: resuming (%d, %d)", iter->token.block, iter->token.offset);

    auto started = millis();
    auto lastStatus = started;
    auto maxBlocks = config->maxBlocks;
    auto success = false;

    do {
        // Make sure the block is loaded up into the cache.
        if (!fkfs_block_ensure(fs, iter->token.block)) {
            fkfs_log("fkfs: unable to ensure block %d", iter->token.block);
            break;
        }

        // Find the next block of the file in the cached memory block.
        auto ptr = fs->buffer + iter->token.offset;
        auto check = fkfs_block_check(fs, ptr);
        if (check == FKFS_OFFSET_SEARCH_STATUS_CRC || check == FKFS_OFFSET_SEARCH_STATUS_GOOD) {
            auto entry = (fkfs_entry_t *)ptr;
            if (check == FKFS_OFFSET_SEARCH_STATUS_GOOD) {
                if (entry->file == iter->token.file) {
                    fkfs_log("fkfs: scanning: DATA (%d, %3d) %d", iter->token.block, iter->token.offset, entry->size);
                    iter->size = entry->size;
                    iter->data = ptr + sizeof(fkfs_entry_t);
                    if (!config->manualNext) {
                        iter->token.offset += entry->available + sizeof(fkfs_entry_t);
                    }
                    success = true;
                    break;
                } else {
                    fkfs_log("fkfs: scanning: file (%d, %3d) (%d)", iter->token.block, iter->token.offset, entry->file);
                }
            }
            else {
                fkfs_log("fkfs: scanning:      (%d, %3d) %s", iter->token.block, iter->token.offset, block_check_str(check));
            }

            iter->token.offset += entry->available + sizeof(fkfs_entry_t);

            if (fkfs_file_iterator_done(fs, iter)) {
                fkfs_log("fkfs: scanning: iterator done (%d)", iter->token.block);
                break;
            }
        }
        else {
            fkfs_log("fkfs: scanning:      (%d, %3d) %s", iter->token.block, iter->token.offset, block_check_str(check));

            iter->token.block++;
            iter->token.offset = 0;

            // When we started we remembered where to stop.
            if (fkfs_file_iterator_done(fs, iter)) {
                fkfs_log("fkfs: scanning: iterator done (%d)", iter->token.block);
                break;
            }

            // Wrap around logic, back to the beginning of the SD. It will now
            // be important to look at priority and for old files.
            if (iter->token.block == fs->numberOfBlocks - 2 || iter->token.block == FKFS_TESTING_LAST_BLOCK) {
                iter->token.block = FKFS_FIRST_BLOCK;
            }

            // See if our self imposed ending terms have been reached.
            auto maxBlocksReached = config->maxBlocks > 0 && --maxBlocks == 0;
            auto maxTimeReached = config->maxTime > 0 && (millis() - started) > config->maxTime;
            if (maxBlocksReached || maxTimeReached) {
                fkfs_log("fkfs: scanning: max reached (%d)", iter->token.block);
                break;
            }
        }

        if (millis() - lastStatus > 1000) {
            fkfs_log("fkfs: scanning: %d / %d", iter->token.block, iter->token.offset);
            lastStatus = millis();
        }
    }
    while (true);

    fs->statistics.iterateTime += millis() - started;

    return success;
}

uint8_t fkfs_log_statistics(fkfs_t *fs) {
    fkfs_log("fkfs: index=%d gen=%d block=%d offset=%d",
             fs->headerIndex, fs->header.generation,
             fs->header.block, fs->header.offset);

    for (uint8_t counter = 0; counter < FKFS_FILES_MAX; ++counter) {
        fkfs_file_t *file = &fs->header.files[counter];
        if (file->name[0] != 0) {
            fkfs_log("fkfs: %d %s", counter, file->name);
        }
    }

    return true;
}
