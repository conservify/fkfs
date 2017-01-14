#ifndef SD_RAW_H_INCLUDED
#define SD_RAW_H_INCLUDED

#include <stdint.h>

typedef struct sd_raw_t {
    uint8_t cs;
    uint8_t status;
    uint8_t type;
    uint16_t offset;
    uint8_t inBlock;
    uint32_t block;
} sd_raw_t;

uint8_t sd_raw_initialize(sd_raw_t *sd, uint8_t pinCs);
uint8_t sd_raw_read_block(sd_raw_t *sd, uint32_t block, uint8_t *destiny);
uint8_t sd_raw_write_block(sd_raw_t *sd, uint32_t block, const uint8_t *source);
uint32_t sd_raw_card_size(sd_raw_t *sd);
uint8_t sd_raw_erase(sd_raw_t *sd, uint32_t firstBlock, uint32_t lastBlock);

const uint16_t SD_RAW_BLOCK_SIZE = 512;

// CMD0 took too long.
uint8_t const SD_CARD_ERROR_CMD0 = 0X1;
// CMD8 was not accepted - not a valid SD card
uint8_t const SD_CARD_ERROR_CMD8 = 0X2;
// card returned an error response for CMD17 (read block)
uint8_t const SD_CARD_ERROR_CMD17 = 0X3;
// card returned an error response for CMD24 (write block)
uint8_t const SD_CARD_ERROR_CMD24 = 0X4;
// WRITE_MULTIPLE_BLOCKS command failed
uint8_t const SD_CARD_ERROR_CMD25 = 0X05;
// Card returned an error response for CMD58 (read OCR)
uint8_t const SD_CARD_ERROR_CMD58 = 0X06;
// SET_WR_BLK_ERASE_COUNT failed
uint8_t const SD_CARD_ERROR_ACMD23 = 0X07;
// Card's ACMD41 initialization process timeout
uint8_t const SD_CARD_ERROR_ACMD41 = 0X08;
// Card returned a bad CSR version field
uint8_t const SD_CARD_ERROR_BAD_CSD = 0X09;
// Erase block group command failed
uint8_t const SD_CARD_ERROR_ERASE = 0X0A;
// Card not capable of single block erase
uint8_t const SD_CARD_ERROR_ERASE_SINGLE_BLOCK = 0X0B;
// Erase sequence timed out
uint8_t const SD_CARD_ERROR_ERASE_TIMEOUT = 0X0C;
// Card returned an error token instead of read data
uint8_t const SD_CARD_ERROR_READ = 0X0D;
// Read CID or CSD failed
uint8_t const SD_CARD_ERROR_READ_REG = 0X0E;
// Reading data took too long.
uint8_t const SD_CARD_ERROR_READ_TIMEOUT = 0X0F;
// Card did not accept STOP_TRAN_TOKEN
uint8_t const SD_CARD_ERROR_STOP_TRAN = 0X10;
// Card returned an error token as a response to a write operation
uint8_t const SD_CARD_ERROR_WRITE = 0X11;
// Attempt to write protected block zero
uint8_t const SD_CARD_ERROR_WRITE_BLOCK_ZERO = 0X12;
// Card did was unprepared for a multiple block write
uint8_t const SD_CARD_ERROR_WRITE_MULTIPLE = 0X13;
// Card returned an error to a CMD13 status check after a write
uint8_t const SD_CARD_ERROR_WRITE_PROGRAMMING = 0X14;
// Write programming took too long.
uint8_t const SD_CARD_ERROR_WRITE_TIMEOUT = 0X15;
// Bad clock rate.
uint8_t const SD_CARD_ERROR_SCK_RATE = 0X16;
// A general error.
uint8_t const SD_CARD_ERROR_GENERAL = 0X17;

// Standard capacity V1 SD card
uint8_t const SD_CARD_TYPE_SD1 = 1;
// Standard capacity V2 SD card
uint8_t const SD_CARD_TYPE_SD2 = 2;
// High Capacity SD card
uint8_t const SD_CARD_TYPE_SDHC = 3;

// Set SCK to max rate of F_CPU/2. See Sd2Card::setSckRate().
uint8_t const SPI_FULL_SPEED = 0;
// Set SCK rate to F_CPU/4. See Sd2Card::setSckRate().
uint8_t const SPI_HALF_SPEED = 1;
// Set SCK rate to F_CPU/8. Sd2Card::setSckRate().
uint8_t const SPI_QUARTER_SPEED = 2;

#endif
