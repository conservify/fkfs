set(SOURCE_FILES
  dma_example.ino
  ../../sd_raw.cpp
  ../../sd_raw_dma.cpp
  ../../fkfs.cpp
  ../../fkfs_log.cpp
)

set(LIBRARIES
  SPI
  SD
)

include_directories(../../)

arduino(fkfs-example-dma "${SOURCE_FILES}" "${LIBRARIES}")
