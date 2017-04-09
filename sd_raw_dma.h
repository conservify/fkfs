#ifndef SD_RAW_DMA_H_INCLUDED
#define SD_RAW_DMA_H_INCLUDED

#include <stdint.h>

#include "sd_raw.h"

#include "utility/dmac.h"
#include "utility/dma.h"

typedef struct sd_raw_dma_t {
    sd_raw_t *sd;
    dma_resource rx_resource;
    dma_resource tx_resource;
    COMPILER_ALIGNED(16) DmacDescriptor tx_descriptor;
    COMPILER_ALIGNED(16) DmacDescriptor rx_descriptor;
} sd_raw_dma_t;

#endif
