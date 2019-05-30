project(fkfs)
cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH $ENV{CMAKE_MODULE_PATH})

add_definitions(-DFKFS_LOGGING)

enable_testing()

add_subdirectory(examples/simple)
add_subdirectory(examples/dma)
