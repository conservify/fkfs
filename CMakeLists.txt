project(fkfs)
cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../arduino-cmake-minimal/cmake/")

include(ArduinoMinimal)

set(Adafruit_ZeroDMA_RECURSE True)

add_definitions(-DFKFS_LOGGING)

enable_testing()

add_subdirectory(test)
add_subdirectory(examples/simple)
add_subdirectory(examples/dma)
