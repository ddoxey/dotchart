cmake_minimum_required(VERSION 3.20)
project(dotchart VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DOTCHART_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

add_library(dotchart_lib STATIC
  src/options.cpp
  src/input.cpp
  src/braille.cpp
  src/render.cpp
)
target_include_directories(dotchart_lib PUBLIC src)

add_executable(dotchart
  src/main.cpp
)
target_link_libraries(dotchart PRIVATE dotchart_lib)

# Reasonable warnings across Clang/GCC/MSVC
if (MSVC)
  target_compile_options(dotchart PRIVATE /W4)
  target_compile_options(dotchart_lib PRIVATE /W4)
  if (DOTCHART_WARNINGS_AS_ERRORS)
    target_compile_options(dotchart PRIVATE /WX)
    target_compile_options(dotchart_lib PRIVATE /WX)
  endif()
else()
  target_compile_options(dotchart PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(dotchart_lib PRIVATE -Wall -Wextra -Wpedantic)
  if (DOTCHART_WARNINGS_AS_ERRORS)
    target_compile_options(dotchart PRIVATE -Werror)
    target_compile_options(dotchart_lib PRIVATE -Werror)
  endif()
endif()

# Tests (GoogleTest)
include(CTest)
if (BUILD_TESTING)
  include(FetchContent)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  )
  FetchContent_MakeAvailable(googletest)

  add_executable(dotchart_tests
    tests/test_braille.cpp
    tests/test_input.cpp
    tests/test_render.cpp
  )
  target_link_libraries(dotchart_tests PRIVATE GTest::gtest_main dotchart_lib)
  target_include_directories(dotchart_tests PRIVATE src)

  include(GoogleTest)
  gtest_discover_tests(dotchart_tests)
endif()

# Install (optional)
include(GNUInstallDirs)
install(TARGETS dotchart RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
