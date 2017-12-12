project(ARDUINO_PROJECTS)
cmake_minimum_required(VERSION 2.6)
set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../arduino-cmake-minimal/cmake/")

include(ArduinoMinimal)

set(Adafruit_ZeroDMA_RECURSE True)

add_subdirectory(examples/simple)
add_subdirectory(examples/dma)
