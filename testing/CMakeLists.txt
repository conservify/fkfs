cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# add_definitions(-DFKFS_LOGGING_VERBOSE)

include_directories(.)
include_directories(..)

add_executable(read read.cpp hal.cpp ../fkfs.cpp)
