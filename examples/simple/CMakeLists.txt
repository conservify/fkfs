include(Arduino)

enable_arduino_toolchain()

set(sources
  main.cpp
  ../../sd_raw.cpp
  ../../fkfs.cpp
  ../../fkfs_log.cpp
  )

add_arduino_library(fkfs-example-simple "${sources}")

find_package(SPI)
target_link_libraries(fkfs-example-simple SPI)

target_include_directories(fkfs-example-simple PRIVATE ../../)

target_compile_options(fkfs-example-simple PRIVATE -Wall -Werror)

add_arduino_firmware(fkfs-example-simple)
