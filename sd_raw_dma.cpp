#include <SPI.h>

#include "sd_raw_dma.h"
#include "sd_raw_internal.h"

static void dma_tx_callback(struct dma_resource *const resource) {

}

static void setup_transfer_descriptor(DmacDescriptor *descriptor, void *source_memory, void *destination_memory, uint32_t transfer_count, dma_beat_size beat_size, bool source_increment, bool destination_increment) {
    uint8_t bytes_per_beat;
    dma_descriptor_config descriptor_config;

    if (beat_size == DMA_BEAT_SIZE_BYTE) bytes_per_beat = 1;
    if (beat_size == DMA_BEAT_SIZE_HWORD) bytes_per_beat = 2;
    if (beat_size == DMA_BEAT_SIZE_WORD) bytes_per_beat = 4;

    dma_descriptor_get_config_defaults(&descriptor_config);

    descriptor_config.beat_size = beat_size;
    descriptor_config.dst_increment_enable = destination_increment;
    descriptor_config.src_increment_enable = source_increment;
    descriptor_config.block_transfer_count = transfer_count;

    descriptor_config.source_address = (uint32_t)source_memory;
    if (source_increment) {
        descriptor_config.source_address += bytes_per_beat * transfer_count; // the *end* of the transfer
    }

    descriptor_config.destination_address = (uint32_t)destination_memory;
    if (destination_increment) {
        descriptor_config.destination_address += bytes_per_beat * transfer_count; // the *end* of the transfer
    }

    dma_descriptor_create(descriptor, &descriptor_config);
}

uint8_t sd_raw_dma_initialize(sd_raw_dma_t *sd_dma, sd_raw_t *sd, uint8_t *source_memory, uint8_t *destination_memory, size_t length) {
    dma_resource_config _config;

    sd_dma->sd = sd;

    dma_get_config_defaults(&_config);

    _config.peripheral_trigger = SERCOM4_DMAC_ID_TX;
    _config.trigger_action = DMA_TRIGGER_ACTON_BEAT;

    dma_allocate(&sd_dma->tx_resource, &_config);
    setup_transfer_descriptor(&sd_dma->tx_descriptor, source_memory, (void *)(&SERCOM4->SPI.DATA.reg), length, DMA_BEAT_SIZE_BYTE, true, false);
    dma_add_descriptor(&sd_dma->tx_resource, &sd_dma->tx_descriptor);

    dma_register_callback(&sd_dma->tx_resource, dma_tx_callback, DMA_CALLBACK_TRANSFER_DONE);
    dma_enable_callback(&sd_dma->tx_resource, DMA_CALLBACK_TRANSFER_DONE);

    dma_get_config_defaults(&_config);

    _config.peripheral_trigger = SERCOM4_DMAC_ID_RX;
    _config.trigger_action = DMA_TRIGGER_ACTON_BEAT;

    dma_allocate(&sd_dma->rx_resource, &_config);
    setup_transfer_descriptor(&sd_dma->rx_descriptor, (void *)(&SERCOM4->SPI.DATA.reg), destination_memory, length, DMA_BEAT_SIZE_BYTE, false, true);
    dma_add_descriptor(&sd_dma->rx_resource, &sd_dma->rx_descriptor);

    return true;
}

uint8_t sd_raw_dma_write_block(sd_raw_dma_t *sd_dma, uint32_t block) {
    if (sd_raw_command(sd_dma->sd, CMD24, block)) {
        return sd_raw_error(sd_dma->sd, SD_CARD_ERROR_CMD24);
    }

    SPI.transfer(DATA_START_BLOCK);

    SPI.beginTransaction(SPISettings(12000000, MSBFIRST, SPI_MODE0));

    dma_start_transfer_job(&sd_dma->tx_resource);

    while (dma_get_job_status(&sd_dma->tx_resource) == STATUS_BUSY);

    SPI.endTransaction();

    SPI.transfer(0xff);
    SPI.transfer(0xff);

    sd_dma->sd->status = SPI.transfer(0xff);

    if ((sd_dma->sd->status & DATA_RES_MASK) != DATA_RES_ACCEPTED) {
        return sd_raw_error(sd_dma->sd, SD_CARD_ERROR_WRITE);
    }

    if (!sd_raw_flush(sd_dma->sd, SD_RAW_WRITE_TIMEOUT)) {
        return sd_raw_error(sd_dma->sd, SD_CARD_ERROR_WRITE_TIMEOUT);
    }

    if (sd_raw_command(sd_dma->sd, CMD13, 0) || SPI.transfer(0xff)) {
        return sd_raw_error(sd_dma->sd, SD_CARD_ERROR_WRITE_PROGRAMMING);
    }

    return true;
}

uint8_t sd_raw_dma_read_block(sd_raw_dma_t *sd_dma, uint32_t block) {
    if (sd_raw_command(sd_dma->sd, CMD17, block)) {
        return sd_raw_error(sd_dma->sd, SD_CARD_ERROR_CMD17);
    }

    if (!sd_wait_start_block(sd_dma->sd)) {
        return sd_raw_error(sd_dma->sd, SD_CARD_ERROR_GENERAL);
    }

    SPI.beginTransaction(SPISettings(12000000, MSBFIRST, SPI_MODE0));

    dma_start_transfer_job(&sd_dma->rx_resource);

    dma_start_transfer_job(&sd_dma->tx_resource);

    while (dma_get_job_status(&sd_dma->tx_resource) == STATUS_BUSY);

    SPI.endTransaction();

    return true;
}
