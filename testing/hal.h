#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstdint>

#include "sd_raw.h"

class FakeSerial {
public:
    void print(const char *str);
};

extern FakeSerial Serial;

uint32_t millis();

uint32_t random(uint32_t max);

uint8_t sd_raw_file_initialize(sd_raw_t *sd, const char *path);

uint8_t sd_raw_file_close(sd_raw_t *sd);
