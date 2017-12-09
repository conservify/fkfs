# set(ASF_DIRECTORY /home/jlewallen/bare/asf-standalone-archive-3.36.1.59/xdk-asf-3.36.1)
# include_directories(${ASF_DIRECTORY}/sam0/drivers/dma)
# include_directories(${ASF_DIRECTORY}/sam0/drivers/dma/module_config)
# include_directories(${ASF_DIRECTORY}/common/utils)
# include_directories(${ASF_DIRECTORY}/sam0/utils)

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
