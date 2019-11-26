find_program(CLANG_FORMAT NAMES clang-format)

if(CLANG_FORMAT)
  message(STATUS "clang-format found")
  add_custom_target(clang_format COMMAND ${CLANG_FORMAT} -i ${cppsrc})
  add_dependencies(${PROJECT_NAME} clang_format)
else()
  message(STATUS "clang-format not found")
endif()

find_program(CMAKE_FORMAT NAMES cmake-format)

if(CMAKE_FORMAT)
  message(STATUS "cmake-format found")
  add_custom_target(cmake_format COMMAND ${CMAKE_FORMAT} -i ${cmakesrc})
  add_dependencies(${PROJECT_NAME} cmake_format)
else()
  message(STATUS "cmake-format not found")
endif()
