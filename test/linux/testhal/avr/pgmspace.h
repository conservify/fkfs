#pragma once

#include <cstdint>

class __FlashStringHelper;

#define PROGMEM

#define pgm_read_byte(p) (*p)
#define memcpy_P memcpy

