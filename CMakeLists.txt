cmake_minimum_required(VERSION 3.13)

project(
  kcc
  VERSION 0.1
  LANGUAGES CXX)

if(NOT (CMAKE_SYSTEM_NAME MATCHES "Linux" OR CMAKE_SYSTEM_NAME MATCHES "Darwin"
       ))
  message(FATAL_ERROR "Only support linux system and macOS")
endif()

if(NOT (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES
                                               "Clang"))
  message(FATAL_ERROR "Only supports gcc and clang")
endif()

set(sanitizer
    "Undefined"
    CACHE
      STRING
      "Compile with a sanitizer. Options are: Address, Thread, Undefined, None")

file(GLOB cppsrc src/*.cpp src/*.h)
file(GLOB cmakesrc CMakeLists.txt cmake/*.cmake)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(compiler-options)
include(sanitizers)

set(CMAKE_AUTOMOC ON)

find_package(fmt REQUIRED)
find_package(LLVM REQUIRED CONFIG)
find_package(Qt5 REQUIRED COMPONENTS Core)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(-DAPP_VERSION="${PROJECT_VERSION}" -DFMT_STRING_ALIAS
                ${LLVM_DEFINITIONS})

add_executable(${PROJECT_NAME} ${cppsrc})

include(format)

target_link_libraries(${PROJECT_NAME} LLVM fmt::fmt Qt5::Core)
