find_program(CLANG_TIDY_EXE NAMES clang-tidy)

if(CLANG_TIDY_EXE)
  set(CMAKE_CXX_CLANG_TIDY
      "${CLANG_TIDY_EXE};--header-filter=${cppsrc}"
      CACHE STRING "" FORCE)
else()
  message(FATAL_ERROR "clang-tidy not found")
endif()
