find_program(CLANG_FORMAT NAMES clang-format)

if(CLANG_FORMAT)
  add_custom_target(clang_format COMMAND ${CLANG_FORMAT} -i ${cppsrc})
  add_dependencies(${PROJECT_NAME} clang_format)
else()
  message(FATAL_ERROR "clang-format not found")
endif()

find_program(CMAKE_FORMAT NAMES cmake-format)

if(CMAKE_FORMAT)
  add_custom_target(cmake_format COMMAND ${CMAKE_FORMAT} -i ${cmakesrc})
  add_dependencies(${PROJECT_NAME} cmake_format)
else()
  message(FATAL_ERROR "cmake-format not found")
endif()
