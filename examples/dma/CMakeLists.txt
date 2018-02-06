set(SOURCE_FILES
  main.cpp
  ../../sd_raw.cpp
  ../../sd_raw_dma.cpp
  ../../fkfs.cpp
  ../../fkfs_log.cpp
  ../../utility/dma.c
)

set(LIBRARIES
  SPI
  SD
)

include_directories(../../)
include_directories(../../utility)

arduino(fkfs-example-dma "${SOURCE_FILES}" "${LIBRARIES}")

target_compile_options(fkfs-example-dma PRIVATE -Wall -Werror)
