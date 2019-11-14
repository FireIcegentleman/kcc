cmake_minimum_required(VERSION 3.13)

project(
  kcc
  VERSION 0.1
  LANGUAGES CXX)

file(GLOB cppsrc src/*.h src/*.cpp)
file(GLOB cmakesrc CMakeLists.txt cmake/*.cmake)

if(CMAKE_BUILD_TYPE MATCHES "Release")
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
                ${LLVM_DEFINITIONS} -DDEV)

add_executable(${PROJECT_NAME} ${cppsrc})

include(format)

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

enable_testing()
aux_source_directory(test/dev TEST_FILES)
set(KCC_EXECUTABLE ${CMAKE_BINARY_DIR}/${PROJECT_NAME})
set(TEST_BINARY_DIR ${CMAKE_BINARY_DIR}/tests)
file(MAKE_DIRECTORY ${TEST_BINARY_DIR})
add_test(NAME check_kcc_executable COMMAND ${KCC_EXECUTABLE} -h)

foreach(TEST_FILE ${TEST_FILES})
  get_filename_component(TEST_FILE_NAME ${TEST_FILE} NAME_WE)
  add_test(NAME "COMPILE--${TEST_FILE_NAME}"
           COMMAND ${KCC_EXECUTABLE} ${CMAKE_SOURCE_DIR}/${TEST_FILE} -o
                   ${TEST_BINARY_DIR}/${TEST_FILE_NAME})
  add_test(NAME "RUN--${TEST_FILE_NAME}"
           COMMAND ${TEST_BINARY_DIR}/${TEST_FILE_NAME})
endforeach()
