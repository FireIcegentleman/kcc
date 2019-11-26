cmake_minimum_required(VERSION 3.13)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified")
  set(CMAKE_BUILD_TYPE
      "Release"
      CACHE STRING "Choose the type of build" FORCE)
endif()

project(
  kcc
  VERSION 0.1
  LANGUAGES CXX)

file(GLOB cppsrc src/*.cpp)
file(GLOB cppheader src/*.h)
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
include(clang-tidy)
include(compiler-options)

set(CMAKE_AUTOMOC ON)

find_package(fmt REQUIRED)
find_package(Clang REQUIRED CONFIG)
find_package(LLVM REQUIRED CONFIG)
find_package(Boost REQUIRED COMPONENTS locale)
find_package(Qt5 REQUIRED COMPONENTS Core)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(-DKCC_VERSION="${PROJECT_VERSION}" -DFMT_STRING_ALIAS
                ${LLVM_DEFINITIONS})

add_executable(${PROJECT_NAME} ${cppsrc})

add_definitions(-DDEV)
if(CMAKE_BUILD_TYPE MATCHES "Debug")
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
