#pragma once

#include <cstring>
#include <cstdint>
#include <cmath>
#include <stddef.h>

#include "avr/pgmspace.h"

#include "Print.h"

typedef bool boolean;
typedef uint8_t byte;

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
//#define abs(x) ((x)>0?(x):-(x))
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define round(x)     ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x) ((x)*(x))
#define isnan(x) std::isnan(x)
#define isinf(x) std::isinf(x)

#define DEC              10
#define LOW             (0x0)
#define HIGH            (0x1)

#define INPUT           (0x0)
#define OUTPUT          (0x1)
#define INPUT_PULLUP    (0x2)
#define INPUT_PULLDOWN  (0x3)

#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define EULER 2.718281828459045235360287471352

#define SERIAL  0x0
#define DISPLAY 0x1

enum BitOrder {
    LSBFIRST = 0,
    MSBFIRST = 1
};

class Stream {
};

class Uart {
public:
    void print(const char *str) {
    }
};

extern Uart Serial;

inline uint32_t random(uint32_t max) {
    return 0;
}

inline uint32_t millis() {
    return 0;
}

inline void delayMicroseconds(uint32_t usecs) {
}

inline void pinMode(uint8_t pin, uint8_t mode) {
}

inline void digitalWrite(uint8_t pin, uint8_t value) {
}

#define SPI_MODE0 0x02

class SPISettings {
public:
    SPISettings(uint32_t clock, BitOrder bitOrder, uint8_t dataMode) {
    }

};

class SPIClass {
public:
    void begin() {
    }

    uint8_t transfer(uint8_t data) {
        return 0xff;
    }

    uint16_t transfer16(uint16_t data) {
    }

    void transfer(void *buf, size_t count) {
    }

    void beginTransaction(SPISettings settings) {
    }

    void endTransaction(void) {
    }

    void setClockDivider(uint32_t divider) {
    }
};

