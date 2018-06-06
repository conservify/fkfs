#ifdef __linux__
#include <linux/unistd.h>       /* for _syscallX macros/related stuff */
#include <linux/kernel.h>       /* for struct sysinfo */
#include <sys/sysinfo.h>
#endif
#ifdef __APPLE__
#include <sys/sysctl.h>
#endif

#include "hal.h"
#include "sd_raw.h"

void FakeSerial::print(const char *str) {
    puts(str);
}

FakeSerial Serial;

struct sd_raw_file_t {
    FILE *fp;
};

#ifdef __linux__
uint32_t millis() {
    struct sysinfo s_info;
    auto error = sysinfo(&s_info);
    return s_info.uptime;
}
#endif

#ifdef __APPLE__
uint32_t millis() {
    struct timeval ts;
    size_t length = sizeof(ts);
    int32_t mib[2] = { CTL_KERN, KERN_BOOTTIME };
    if (sysctl(mib, 2, &ts, &length, NULL, 0) == 0) {
        return static_cast<unsigned long long>(ts.tv_sec) * 1000ULL +
               static_cast<unsigned long long>(ts.tv_usec) / 1000ULL;
    }
    return 0;
}
#endif

uint32_t random(uint32_t max) {
    return rand() % max;
}

sd_raw_file_t *sd_raw_file(sd_raw_t *sd) {
    return (sd_raw_file_t *)sd;
}

uint8_t sd_raw_file_initialize(sd_raw_t *sd, const char *path) {
    auto sdf = sd_raw_file(sd);
    sdf->fp = fopen(path, "r+b");
    if (sdf->fp == nullptr) {
        sdf->fp = fopen(path, "w+b");
        if (sdf->fp == nullptr) {
            return false;
        }
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

    if (fseek(sdf->fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "error: Unable to seek to block %d\n", block);
        return false;
    }

    auto lengthOfFile = ftell(sdf->fp);
    auto position = SD_RAW_BLOCK_SIZE * block;

    if (position + SD_RAW_BLOCK_SIZE > lengthOfFile) {
        return true;
    }

    if (fseek(sdf->fp, position, SEEK_SET) != 0) {
        fprintf(stderr, "error: Unable to seek to block %d\n", block);
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

    if (fseek(sdf->fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "error: Unable to seek to block %d\n", block);
        return false;
    }

    auto lengthOfFile = ftell(sdf->fp);
    auto position = SD_RAW_BLOCK_SIZE * block;

    printf("writing %d\n", block);

    if (fseek(sdf->fp, position, SEEK_SET) != 0) {
        fprintf(stderr, "error: Unable to seek to block %d\n", block);
        return false;
    }

    if (fwrite(source, 1, SD_RAW_BLOCK_SIZE, sdf->fp) != SD_RAW_BLOCK_SIZE) {
        fprintf(stderr, "error: Unable to write block %d\n", block);
        return false;
    }
    return true;
}

uint32_t sd_raw_card_size(sd_raw_t *sd) {
    auto sdf = sd_raw_file(sd);
    return 0;
}

uint8_t sd_raw_erase(sd_raw_t *sd, uint32_t firstBlock, uint32_t lastBlock) {
    auto sdf = sd_raw_file(sd);
    return 0;
}
