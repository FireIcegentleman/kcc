cmake_minimum_required(VERSION 3.13)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified")
  set(CMAKE_BUILD_TYPE
      "Release"
      CACHE STRING "Choose the type of build" FORCE)
endif()

project(
  kcc
  VERSION 0.2
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

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  add_definitions(-DDEV)
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

enable_testing()
set(TEST_BINARY_DIR ${CMAKE_BINARY_DIR}/tests)
file(MAKE_DIRECTORY ${TEST_BINARY_DIR})

set(KCC_EXECUTABLE ${CMAKE_BINARY_DIR}/${PROJECT_NAME})
add_test(NAME check_kcc_executable COMMAND ${KCC_EXECUTABLE} -h)

aux_source_directory(${CMAKE_SOURCE_DIR}/test/usual USUAL_FILES)

foreach(USUAL_FILE ${USUAL_FILES})
  get_filename_component(USUAL_FILE_NAME ${USUAL_FILE} NAME_WE)
  if(USUAL_FILE_NAME MATCHES "testmain")
    continue()
  endif()

  add_test(
    NAME "COMPILE--${USUAL_FILE_NAME}"
    COMMAND ${KCC_EXECUTABLE} ${USUAL_FILE}
            ${CMAKE_SOURCE_DIR}/test/usual/testmain.c -O3 -o
            ${TEST_BINARY_DIR}/${USUAL_FILE_NAME})
  add_test(NAME "RUN--${USUAL_FILE_NAME}"
           COMMAND ${TEST_BINARY_DIR}/${USUAL_FILE_NAME})
endforeach()

add_test(
  NAME "COMPILE--8CC"
  COMMAND ${KCC_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/8cc/*.c -O3
          -DBUILD_DIR=\"${CMAKE_SOURCE_DIR}/test/8cc\" -o
          ${TEST_BINARY_DIR}/8cc)
add_test(NAME check_8cc_executable COMMAND ${TEST_BINARY_DIR}/8cc -h)

set(TEST_OBJ_DIR ${CMAKE_BINARY_DIR}/objs)
file(MAKE_DIRECTORY ${TEST_OBJ_DIR})
aux_source_directory(${CMAKE_SOURCE_DIR}/test/8cc 8CC_FILES)

foreach(8CC_FILE ${8CC_FILES})
  get_filename_component(8CC_FILE_NAME ${8CC_FILE} NAME_WE)

  add_test(
    NAME "COMPILE--${8CC_FILE_NAME}_8cc"
    COMMAND ${TEST_BINARY_DIR}/8cc ${8CC_FILE}
            -DBUILD_DIR=\"${CMAKE_SOURCE_DIR}/test/8cc\" -c -o
            ${TEST_OBJ_DIR}/${8CC_FILE_NAME}.o)
endforeach()

add_test(
  NAME "COMPILE--LUA"
  COMMAND
    ${KCC_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/lua/*.c -O3
    -DLUA_USER_H=\"ltests.h\" -DLUA_USE_LINUX -DLUA_COMPAT_5_2 -ldl -lreadline
    -lm -o ${TEST_BINARY_DIR}/lua)
add_test(NAME check_lua_executable COMMAND ${TEST_BINARY_DIR}/lua -v)

add_test(
  NAME lua_test
  COMMAND ${TEST_BINARY_DIR}/lua ${CMAKE_SOURCE_DIR}/test/lua/testes/all.lua
          args
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test/lua/testes)

add_test(
  NAME "COMPILE--SQLITE"
  COMMAND
    ${KCC_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/sqlite/*.c -O3 -DNDEBUG
    -lpthread -ldl -o ${TEST_BINARY_DIR}/sqlite -DSQLITE_DEFAULT_MEMSTATUS=0
    -DSQLITE_DQS=0 -DSQLITE_LIKE_DOESNT_MATCH_BLOBS -DSQLITE_MAX_EXPR_DEPTH=0
    -DSQLITE_OMIT_DECLTYPE -DSQLITE_OMIT_DEPRECATED -DSQLITE_USE_ALLOCA)
add_test(NAME check_sqlite_executable COMMAND ${TEST_BINARY_DIR}/sqlite
                                              -version)
