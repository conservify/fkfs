#include "fkfs_log.h"

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

uint8_t fkfs_log_append_binary(fkfs_log_t *log, uint8_t *ptr, size_t length, bool canSplit) {
    if (!canSplit && (FKFS_MAXIMUM_BLOCK_SIZE - log->position) < length) {
        if (length >= FKFS_MAXIMUM_BLOCK_SIZE) {
            return false;
        }
        if (!fkfs_log_flush(log)) {
            return false;
        }
    }

    size_t required = length;
    while (required > 0) {
        size_t available = FKFS_MAXIMUM_BLOCK_SIZE - log->position;
        size_t copy = required > available ? available : required;

        memcpy((uint8_t *)log->buffer + log->position, ptr, copy);

        log->position += copy;
        required -= copy;
        ptr += copy;

        if (log->position >= FKFS_MAXIMUM_BLOCK_SIZE) {
            if (!fkfs_log_flush(log)) {
                return false;
            }
        }
    }
}

uint8_t fkfs_log_append(fkfs_log_t *log, const char *str) {
    size_t required = strlen(str);
    return fkfs_log_append_binary(log, (uint8_t *)str, required, true);
}

uint8_t fkfs_log_printf(fkfs_log_t *log, const char *format, ...) {
    char incoming[FKFS_MAXIMUM_BLOCK_SIZE];
    va_list args;
    va_start(args, format);
    vsnprintf(incoming, sizeof(incoming), format, args);
    va_end(args);

    return fkfs_log_append(log, incoming);
}
