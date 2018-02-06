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

int extract(fkfs_t *fs, uint8_t id, std::string filename) {
    fkfs_file_iter_t iter = { 0 };
    fkfs_iterator_token_t token =  { 0x0 };
    fkfs_iterator_config_t config = {
        .maxBlocks = UINT32_MAX,
        .maxTime = 0,
    };

    auto fp = fopen(filename.c_str(), "w");
    if (fp == nullptr) {
        return false;
    }

    printf("Extracting %s...\n", filename.c_str());

    while (fkfs_file_iterate(fs, id, &config, &iter, &token)) {
        char buffer[iter.size + 1];
        memcpy(buffer, iter.data, iter.size);
        buffer[iter.size] = 0;
        fprintf(fp, "%s", buffer);

        printf("Block: %d\n", iter.size);
    }

    fclose(fp);

    return true;
}

int main(int argc, const char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <image> <directory>\n", argv[0]);
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

    std::string slash = "/";
    std::string path = argv[2] + slash;

    if (true) {
        if (!extract(&fs, FKFS_FILE_LOG, path + "FK.LOG")) {
            fprintf(stderr, "error: Unable to extract file.\n");
            return 2;
        }
    }

    if (!extract(&fs, FKFS_FILE_DATA, path + "DATA.BIN")) {
        fprintf(stderr, "error: Unable to extract file.\n");
        return 2;
    }

    sd_raw_file_close(&fs.sd);

    return 0;
}
