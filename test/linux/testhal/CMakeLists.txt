file(GLOB SRCS *.cpp)

add_executable(testcommon ${SRCS})

include(ArduinoMinimal)

read_arduino_libraries(PROJECT_LIBRARIES ${CMAKE_CURRENT_SOURCE_DIR})
setup_libraries(LIBRARY_INFO "testcommon" "" "" "" "${PROJECT_LIBRARIES}")

foreach(key ${LIBRARY_INFO})
  set(LIB_INCLUDES "${LIB_INCLUDES};${${key}_INCLUDES}")

  list(GET "${key}_INFO" 3 HEADERS_ONLY)
  list(GET "${key}_INFO" 4 LIB_TARGET_NAME)

  if(NOT HEADERS_ONLY)
    message("-- Dependency ${TARGET_NAME} ${LIB_TARGET_NAME}")
    add_dependencies(testcommon ${LIB_TARGET_NAME})
    list(APPEND LIBRARY_DEPS "${LIBRARY_OUTPUT_DIRECTORY}/lib${LIB_TARGET_NAME}.a")
  endif()
endforeach(key)

foreach(key ${LIBRARY_INFO})
  list(GET "${key}_INFO" 3 HEADERS_ONLY)
  list(GET "${key}_INFO" 4 LIB_TARGET_NAME)

  if(NOT HEADERS_ONLY)
    target_include_directories(${LIB_TARGET_NAME} PUBLIC "${LIB_INCLUDES}")
  endif()
endforeach(key)

target_include_directories(testcommon PUBLIC "${LIB_INCLUDES}")

target_link_libraries(testcommon
    libgtest
    libgmock
)

set_target_properties(testcommon PROPERTIES C_STANDARD 11)
set_target_properties(testcommon PROPERTIES CXX_STANDARD 11)

target_include_directories(testcommon PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_test(NAME testcommon COMMAND testcommon)
