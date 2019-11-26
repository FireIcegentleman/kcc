find_program(CLANG_TIDY_EXE NAMES clang-tidy)

if(CLANG_TIDY_EXE)
  message(STATUS "clang-tidy found")
  add_custom_target(clang_tidy COMMAND ${CLANG_TIDY_EXE} ${cppsrc}
                                       --header-filter=${cppheader})
else()
  message(STATUS "clang-tidy not found")
endif()
