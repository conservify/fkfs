#include <linux/unistd.h>       /* for _syscallX macros/related stuff */
#include <linux/kernel.h>       /* for struct sysinfo */
#include <sys/sysinfo.h>

#include "hal.h"
#include "sd_raw.h"

void FakeSerial::print(const char *str) {
    puts(str);
}

FakeSerial Serial;

struct sd_raw_file_t {
    FILE *fp;
};

uint32_t millis() {
    struct sysinfo s_info;
    auto error = sysinfo(&s_info);
    return s_info.uptime;
}

uint32_t random(uint32_t max) {
    return rand() % max;
}

sd_raw_file_t *sd_raw_file(sd_raw_t *sd) {
    return (sd_raw_file_t *)sd;
}

uint8_t sd_raw_file_initialize(sd_raw_t *sd, const char *path) {
    auto sdf = sd_raw_file(sd);
    sdf->fp = fopen(path, "rw");
    if (sdf->fp == nullptr) {
        return false;
    }

    return true;
}

uint8_t sd_raw_file_close(sd_raw_t *sd) {
    auto sdf = sd_raw_file(sd);
    fclose(sdf->fp);
    return true;
}

uint8_t sd_raw_read_block(sd_raw_t *sd, uint32_t block, uint8_t *destiny) {
    auto sdf = sd_raw_file(sd);
    if (fseek(sdf->fp, SD_RAW_BLOCK_SIZE * block, SEEK_SET) != 0) {
        fprintf(stderr, "error: Unable to seed to block %d\n", block);
        return false;
    }

    if (fread(destiny, 1, SD_RAW_BLOCK_SIZE, sdf->fp) != SD_RAW_BLOCK_SIZE) {
        fprintf(stderr, "error: Unable to read block %d\n", block);
        return false;
    }
    return true;
}

uint8_t sd_raw_write_block(sd_raw_t *sd, uint32_t block, const uint8_t *source) {
    auto sdf = sd_raw_file(sd);
    return 0;
}

uint32_t sd_raw_card_size(sd_raw_t *sd) {
    auto sdf = sd_raw_file(sd);
    return 0;
}

uint8_t sd_raw_erase(sd_raw_t *sd, uint32_t firstBlock, uint32_t lastBlock) {
    auto sdf = sd_raw_file(sd);
    return 0;
}
