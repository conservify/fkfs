set(SOURCE_FILES
  main.cpp
  ../../sd_raw.cpp
  ../../fkfs.cpp
  ../../fkfs_log.cpp
)

set(LIBRARIES
  SPI
  SD
)

include_directories(../../)

arduino(fkfs-example-simple "${SOURCE_FILES}" "${LIBRARIES}")

target_compile_options(fkfs-example-simple PRIVATE -Wall -Werror)
