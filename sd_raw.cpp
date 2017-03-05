#include "sd_raw.h"
#include "sd_raw_internal.h"

#include <SPI.h>

const uint32_t SD_RAW_INIT_TIMEOUT = 2 * 1000;
const uint32_t SD_RAW_READ_TIMEOUT = 300;
const uint32_t SD_RAW_WRITE_TIMEOUT = 600;
const uint32_t SD_RAW_ERASE_TIMEOUT = 10 * 1000;

static uint8_t sd_raw_cs_high(sd_raw_t *sd) {
    digitalWrite(sd->cs, HIGH);
    return true;
}

static uint8_t sd_raw_cs_low(sd_raw_t *sd) {
    digitalWrite(sd->cs, LOW);
    return true;
}

static uint8_t sd_raw_spi_read() {
    return SPI.transfer(0xff);
}

static uint8_t sd_raw_spi_write(uint8_t value) {
    return SPI.transfer(value);
}

static uint8_t sd_raw_flush(sd_raw_t *sd, uint16_t timeoutMs) {
    uint16_t t0 = millis();

    do {
        if (sd_raw_spi_read() == 0xff) {
            return true;
        }
    }
    while (((uint16_t)millis() - t0) < timeoutMs);

    return false;
}

static uint8_t sd_raw_read_end(sd_raw_t *sd) {
    if (sd->inBlock) {
        while (sd->offset++ < SD_RAW_BLOCK_SIZE + 2) { // I think this is block size + crc bytes.
            sd_raw_spi_read();
        }

        sd_raw_cs_high(sd);
        sd->inBlock = false;
    }
    return true;
}

static uint8_t sd_raw_command(sd_raw_t *sd, uint8_t command, uint32_t arg) {
    sd_raw_read_end(sd);
    sd_raw_cs_low(sd);
    sd_raw_flush(sd, 300);

    sd_raw_spi_write(command | 0x40);

    for (int8_t s = 24; s >= 0; s -= 8) {
        sd_raw_spi_write(arg >> s);
    }

    uint8_t crc = 0xff;
    if (command == CMD0) crc = 0x95;  // Correct crc for CMD0 with arg 0
    if (command == CMD8) crc = 0x87;  // Correct crc for CMD8 with arg 0x1AA
    sd_raw_spi_write(crc);

    for (uint8_t i = 0; ((sd->status = sd_raw_spi_read()) & 0x80) && i != 0xff; i++) {
    }
    return sd->status;
}

static uint8_t sd_raw_acommand(sd_raw_t *sd, uint8_t command, uint32_t arg) {
    sd_raw_command(sd, CMD55, 0);
    return sd_raw_command(sd, command, arg);
}

static uint8_t sd_raw_error(sd_raw_t *sd, uint32_t error) {
    sd_raw_cs_high(sd);
    sd->status = error;
    return false;
}

static uint8_t sd_raw_spi_configure() {
    SPI.begin();
    SPI.setClockDivider(255);

    // Card takes 74 clock cycles to start up.
    for (uint8_t i = 0; i < 10; i++) {
        SPI.transfer(0xff);
    }

    SPI.setClockDivider(SPI_FULL_SPEED);

    return true;
}

uint8_t sd_raw_initialize(sd_raw_t *sd, uint8_t pinCs) {
    sd->cs = pinCs;

    pinMode(sd->cs, OUTPUT);
    sd_raw_cs_high(sd);

    sd_raw_spi_configure();

    sd_raw_cs_low(sd);

    uint32_t t0 = millis();

    // Command to go idle in SPI mode
    while ((sd->status = sd_raw_command(sd, CMD0, 0)) != R1_IDLE_STATE) {
        if (((uint16_t)millis() - t0) > SD_RAW_INIT_TIMEOUT) {
            return sd_raw_error(sd, SD_CARD_ERROR_CMD0);
        }
    }

    // Check SD version
    if ((sd_raw_command(sd, CMD8, 0x1aa) & R1_ILLEGAL_COMMAND)) {
        sd->type = SD_CARD_TYPE_SD1;
    } else {
        // Only need last byte of r7 response
        for (uint8_t i = 0; i < 4; i++) {
            sd->status = sd_raw_spi_read();
        }

        if (sd->status != 0xAA) {
            return sd_raw_error(sd, SD_CARD_ERROR_CMD8);
        }
        sd->type = SD_CARD_TYPE_SD2;
    }

    // Initialize card and send host supports SDHC if SD2
    uint32_t arg = sd->type == SD_CARD_TYPE_SD2 ? 0x40000000 : 0;

    while ((sd->status = sd_raw_acommand(sd, ACMD41, arg)) != R1_READY_STATE) {
        // Check for timeout
        if (((uint16_t)millis() - t0) > SD_RAW_INIT_TIMEOUT) {
            return sd_raw_error(sd, SD_CARD_ERROR_ACMD41);
        }
    }

    // If SD2 read OCR register to check for SDHC card
    if (sd->type == SD_CARD_TYPE_SD2) {
        if (sd_raw_command(sd, CMD58, 0)) {
            return sd_raw_error(sd, SD_CARD_ERROR_CMD58);
        }

        if ((sd_raw_spi_read() & 0xc0) == 0xc0) {
            sd->type = SD_CARD_TYPE_SDHC;
        }

        // Discard rest of ocr - contains allowed voltage range
        for (uint8_t i = 0; i < 3; i++) {
            sd_raw_spi_read();
        }
    }

    sd_raw_cs_high(sd);

    return true;
}

static uint8_t sd_wait_start_block(sd_raw_t *sd) {
    uint16_t t0 = millis();

    while ((sd->status = sd_raw_spi_read()) == 0xff) {
        if (((uint16_t)millis() - t0) > SD_RAW_READ_TIMEOUT) {
            return sd_raw_error(sd, SD_CARD_ERROR_READ_TIMEOUT);
        }
    }

    if (sd->status != DATA_START_BLOCK) {
        return sd_raw_error(sd, SD_CARD_ERROR_READ);
    }

    return true;
}

static uint8_t sd_raw_read_register(sd_raw_t *sd, uint8_t command, void *buffer) {
    uint8_t *destiny = reinterpret_cast<uint8_t*>(buffer);

    if (sd_raw_command(sd, command, 0)) {
        return sd_raw_error(sd, SD_CARD_ERROR_READ_REG);
    }

    if (!sd_wait_start_block(sd)) {
        return sd_raw_error(sd, SD_CARD_ERROR_GENERAL);
    }

    for (uint16_t i = 0; i < 16; i++) {
        destiny[i] = sd_raw_spi_read();
    }

    sd_raw_spi_read(); // CRC byte
    sd_raw_spi_read(); // CRC byte

    sd_raw_cs_high(sd);

    return true;
}

static uint8_t sd_raw_read_cid(sd_raw_t *sd, cid_t* cid) {
    return sd_raw_read_register(sd, CMD10, cid);
}

static uint8_t sd_raw_read_csd(sd_raw_t *sd, csd_t* csd) {
    return sd_raw_read_register(sd, CMD9, csd);
}

static uint8_t sd_raw_read_data(sd_raw_t *sd, uint32_t block, uint16_t offset, uint16_t size, uint8_t *destiny) {
    const uint8_t partialBlockRead = false;

    if (size == 0) {
        return true;
    }

    if ((size + offset) > SD_RAW_BLOCK_SIZE) {
        return sd_raw_error(sd, SD_CARD_ERROR_GENERAL);
    }

    if (!sd->inBlock || block != sd->block || offset < sd->offset) {
        sd->block = block;

        if (sd->type != SD_CARD_TYPE_SDHC) {
            block <<= 9;
        }

        if (sd_raw_command(sd, CMD17, block)) {
            return sd_raw_error(sd, SD_CARD_ERROR_CMD17);
        }

        if (!sd_wait_start_block(sd)) {
            return sd_raw_error(sd, SD_CARD_ERROR_GENERAL);
        }

        sd->offset = 0;
        sd->inBlock = true;
    }

    // Skip data before offset
    for (; sd->offset < offset; sd->offset++) {
        sd_raw_spi_read();
    }
    for (uint16_t i = 0; i < size; i++) {
        destiny[i] = sd_raw_spi_read();
    }

    sd->offset += size;
    if (!partialBlockRead || sd->offset >= SD_RAW_BLOCK_SIZE) {
        sd_raw_read_end(sd);
    }
    return true;
}

uint8_t sd_raw_read_block(sd_raw_t *sd, uint32_t block, uint8_t *destiny) {
    return sd_raw_read_data(sd, block, 0, SD_RAW_BLOCK_SIZE, destiny);
}

static uint8_t sd_raw_write_data(sd_raw_t *sd, uint8_t token, const uint8_t *source) {
    // CRC16 checksum is supposed to be ignored in SPI mode (unless
    // explicitly enabled) and a dummy value is normally written.
    // A few funny cards (e.g. Eye-Fi X2) expect a valid CRC anyway.
    // Call setCRC(true) to enable CRC16 checksum on block writes.
    // This has a noticeable impact on write speed. :(
    // NOTE: We just aren't going to support these cards. -jlewallen
    int16_t crc = 0xffff; // Dummy value

    #ifdef SD_RAW_CRC_SUPPORT
    if (sd->writeCrc) {
        int16_t i, x;
        // CRC16 code via Scott Dattalo www.dattalo.com
        for (crc = i = 0; i < SD_RAW_BLOCK_SIZE; i++) {
            x   = ((crc >> 8) ^ source[i]) & 0xff;
            x  ^= x >> 4;
            crc = (crc << 8) ^ (x << 12) ^ (x << 5) ^ x;
        }
    }
    #endif // SD_RAW_CRC_SUPPORT

    sd_raw_spi_write(token);

    for (uint16_t i = 0; i < SD_RAW_BLOCK_SIZE; i++) {
        sd_raw_spi_write(source[i]);
    }

    sd_raw_spi_write(crc >> 8);
    sd_raw_spi_write(crc);

    sd->status = sd_raw_spi_read();

    if ((sd->status & DATA_RES_MASK) != DATA_RES_ACCEPTED) {
        return sd_raw_error(sd, SD_CARD_ERROR_WRITE);
    }

    return true;
}

uint8_t sd_raw_write_block(sd_raw_t *sd, uint32_t block, const uint8_t *source) {
    #if SD_PROTECT_BLOCK_ZERO
    if (block == 0) {
        return sd_raw_error(sd, SD_CARD_ERROR_WRITE_BLOCK_ZERO);
    }
    #endif // SD_PROTECT_BLOCK_ZERO

    if (sd->type != SD_CARD_TYPE_SDHC) {
        block <<= 9;
    }

    if (sd_raw_command(sd, CMD24, block)) {
        return sd_raw_error(sd, SD_CARD_ERROR_CMD24);
    }

    if (!sd_raw_write_data(sd, DATA_START_BLOCK, source)) {
        return sd_raw_error(sd, SD_CARD_ERROR_GENERAL);
    }

    // Wait for flash programming to complete
    if (!sd_raw_flush(sd, SD_RAW_WRITE_TIMEOUT)) {
        return sd_raw_error(sd, SD_CARD_ERROR_WRITE_TIMEOUT);
    }

    // Response is r2 so get and check two bytes for nonzero
    if (sd_raw_command(sd, CMD13, 0) || sd_raw_spi_read()) {
        return sd_raw_error(sd, SD_CARD_ERROR_WRITE_PROGRAMMING);
    }

    sd_raw_cs_high(sd);
    return true;
}

uint32_t sd_raw_card_size(sd_raw_t *sd) {
    csd_t csd;

    if (!sd_raw_read_csd(sd, &csd)) {
        return 0;
    }

    if (csd.v1.csd_ver == 0) {
        uint8_t readBlLen = csd.v1.read_bl_len;
        uint16_t cSize = (csd.v1.c_size_high << 10) | (csd.v1.c_size_mid << 2) | csd.v1.c_size_low;
        uint8_t cSizeMult = (csd.v1.c_size_mult_high << 1) | csd.v1.c_size_mult_low;
        return (uint32_t)(cSize + 1) << (cSizeMult + readBlLen - 7);
    }
    else if (csd.v2.csd_ver == 1) {
        uint32_t cSize = ((uint32_t)csd.v2.c_size_high << 16) | (csd.v2.c_size_mid << 8) | csd.v2.c_size_low;
        return (cSize + 1) * 1024;
    }
    else {
        sd_raw_error(sd, SD_CARD_ERROR_BAD_CSD);
        return 0;
    }
}

static uint8_t sd_raw_erase_single_block_enabled(sd_raw_t *sd) {
    csd_t csd;
    return sd_raw_read_csd(sd, &csd) ? csd.v1.erase_blk_en : 0;
}

uint8_t sd_raw_erase(sd_raw_t *sd, uint32_t firstBlock, uint32_t lastBlock) {
    if (!sd_raw_erase_single_block_enabled(sd)) {
        return sd_raw_error(sd, SD_CARD_ERROR_ERASE_SINGLE_BLOCK);
    }

    if (sd->type != SD_CARD_TYPE_SDHC) {
        firstBlock <<= 9;
        lastBlock <<= 9;
    }

    if (sd_raw_command(sd, CMD32, firstBlock) ||
        sd_raw_command(sd, CMD33, lastBlock) ||
        sd_raw_command(sd, CMD38, 0)) {
        return sd_raw_error(sd, SD_CARD_ERROR_ERASE);
    }

    if (!sd_raw_flush(sd, SD_RAW_ERASE_TIMEOUT)) {
        return sd_raw_error(sd, SD_CARD_ERROR_ERASE_TIMEOUT);
    }

    sd_raw_cs_high(sd);
    return true;
}
