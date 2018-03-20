#ifndef SD_RAW_DMA_H_INCLUDED
#define SD_RAW_DMA_H_INCLUDED

#ifdef ARDUINO

#include <stdint.h>

#include "sd_raw.h"
#include "utility/dma.h"

typedef struct sd_raw_dma_t {
    sd_raw_t *sd;
    dma_resource rx_resource;
    dma_resource tx_resource;
    COMPILER_ALIGNED(16) DmacDescriptor tx_descriptor;
    COMPILER_ALIGNED(16) DmacDescriptor rx_descriptor;
} sd_raw_dma_t;

uint8_t sd_raw_dma_initialize(sd_raw_dma_t *sd_dma, sd_raw_t *sd, uint8_t *source_memory, uint8_t *destination_memory, size_t length);

uint8_t sd_raw_dma_write_block(sd_raw_dma_t *sd_dma, uint32_t block);

uint8_t sd_raw_dma_read_block(sd_raw_dma_t *sd_dma, uint32_t block);

#endif

#endif
