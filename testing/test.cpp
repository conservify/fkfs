#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <string>

#include "Arduino.h"
#include "sd_raw.h"
#include "fkfs.h"

static constexpr uint8_t FKFS_FILE_LOG = 0;
static constexpr uint8_t FKFS_FILE_DATA = 1;
static constexpr uint8_t FKFS_FILE_PRIORITY_LOWEST = 255;
static constexpr uint8_t FKFS_FILE_PRIORITY_HIGHEST = 0;

int main(int argc, const char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <image>\n", argv[0]);
        return 2;
    }

    fkfs_t fs;
    if (!fkfs_create(&fs)) {
        return false;
    }

    if (!sd_raw_file_initialize(&fs.sd, argv[1])) {
        fprintf(stderr, "error: Unable to open file.\n");
        return 2;
    }

    if (!fkfs_initialize_file(&fs, FKFS_FILE_LOG, FKFS_FILE_PRIORITY_LOWEST, true, "FK.LOG")) {
        fprintf(stderr, "error: Unable to initialize file.\n");
        return 2;
    }

    if (!fkfs_initialize_file(&fs, FKFS_FILE_DATA, FKFS_FILE_PRIORITY_HIGHEST, true, "DATA.BIN")) {
        fprintf(stderr, "error: Unable to initialize file.\n");
        return 2;
    }

    if (!fkfs_initialize(&fs, false)) {
        fprintf(stderr, "error: Unable to initialize fkfs.\n");
        return 2;
    }

    fkfs_log_statistics(&fs);

    const char *buffer = "Hello, world!";
    if (!fkfs_file_append(&fs, FKFS_FILE_LOG, strlen(buffer), (uint8_t *)buffer)) {
        fprintf(stderr, "error: Unable to append to file.\n");
        return 0;
    }

    fkfs_flush(&fs);

    sd_raw_file_close(&fs.sd);

    return 0;
}
