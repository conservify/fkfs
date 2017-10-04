project(ARDUINO_PROJECTS)
cmake_minimum_required(VERSION 2.6)
set(CMAKE_VERBOSE_MAKEFILE on)

include($ENV{HOME}/conservify/arduino-cmake-minimal/cmake/ArduinoMinimal.cmake)

set(Adafruit_ZeroDMA_RECURSE True)

add_subdirectory(examples/simple)
add_subdirectory(examples/dma)
