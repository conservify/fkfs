#ifndef FKFS_LOG_H_INCLUDED
#define FKFS_LOG_H_INCLUDED

#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>

#include "fkfs.h"

typedef struct fkfs_log_t {
    fkfs_t *fs;
    uint8_t file;
    size_t position;
    char buffer[FKFS_MAXIMUM_BLOCK_SIZE];
} fkfs_log_t;

uint8_t fkfs_log_initialize(fkfs_log_t *log, fkfs_t *fs, uint8_t file);

uint8_t fkfs_log_flush(fkfs_log_t *log);

uint8_t fkfs_log_append_binary(fkfs_log_t *log, uint8_t *ptr, size_t length);

uint8_t fkfs_log_append(fkfs_log_t *log, const char *str);

uint8_t fkfs_log_printf(fkfs_log_t *log, const char *format, ...);

#endif
