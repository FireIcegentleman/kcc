cmake_minimum_required(VERSION 3.13)

project(
  kcc
  VERSION 0.1
  LANGUAGES CXX)

file(GLOB cppsrc src/*.h src/*.cpp)
file(GLOB cmakesrc CMakeLists.txt cmake/*.cmake)

if(NOT (CMAKE_BUILD_TYPE MATCHES "Debug"))
  set(sanitizer "None")
else()
  # Address Thread Undefined None
  set(sanitizer "Undefined")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(check)
include(sanitizers)
include(compiler-options)

set(CMAKE_AUTOMOC ON)

find_package(fmt REQUIRED)
find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)
find_package(Qt5 REQUIRED COMPONENTS Core)
find_package(Boost REQUIRED COMPONENTS locale)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(-DKCC_VERSION="${PROJECT_VERSION}" -DFMT_STRING_ALIAS
                ${LLVM_DEFINITIONS})

add_executable(${PROJECT_NAME} ${cppsrc})

add_definitions(-DDEV)
if(CMAKE_BUILD_TYPE MATCHES "Debug")
  add_definitions(-DDEV)
  message(STATUS "cmake-format found")
  include(format)
endif()

target_link_libraries(
  ${PROJECT_NAME}
  ${Boost_LIBRARIES}
  Qt5::Core
  fmt::fmt
  clangLex
  clangDriver
  clangFrontend
  clangBasic
  clangSerialization
  LLVM
  lldELF)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

add_custom_target(uninstall COMMAND rm
                                    ${CMAKE_INSTALL_PREFIX}/bin/${PROJECT_NAME})
