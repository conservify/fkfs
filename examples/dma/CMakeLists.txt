include(Arduino)

enable_arduino_toolchain()

set(sources
  main.cpp
  ../../sd_raw.cpp
  ../../sd_raw_dma.cpp
  ../../fkfs.cpp
  ../../fkfs_log.cpp
  ../../utility/dma.c
)

add_arduino_library(fkfs-example-dma "${sources}")

find_package(SPI)
target_link_libraries(fkfs-example-dma SPI)

find_package(Adafruit_ASFcore)
target_link_libraries(fkfs-example-dma Adafruit_ASFcore)

target_include_directories(fkfs-example-dma PRIVATE ../../)

target_compile_options(fkfs-example-dma PRIVATE -Wall -Werror)

add_arduino_firmware(fkfs-example-dma)
