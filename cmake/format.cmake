find_program(CMAKE_FORMAT NAMES cmake-format)

if(CMAKE_FORMAT)
  add_custom_target(cmake_format COMMAND ${CMAKE_FORMAT} -i ${cmakesrc})
  add_dependencies(${PROJECT_NAME} cmake_format)
else()
  message(STATUS "cmake-format not found")
endif()
