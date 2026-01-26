cmake_minimum_required(VERSION 3.20)
project(dotchart VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DOTCHART_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

add_executable(dotchart
  src/main.cpp
  src/options.cpp
  src/input.cpp
  src/braille.cpp
  src/render.cpp
)

target_include_directories(dotchart PRIVATE src)

# Reasonable warnings across Clang/GCC/MSVC
if (MSVC)
  target_compile_options(dotchart PRIVATE /W4)
  if (DOTCHART_WARNINGS_AS_ERRORS)
    target_compile_options(dotchart PRIVATE /WX)
  endif()
else()
  target_compile_options(dotchart PRIVATE -Wall -Wextra -Wpedantic)
  if (DOTCHART_WARNINGS_AS_ERRORS)
    target_compile_options(dotchart PRIVATE -Werror)
  endif()
endif()

# Install (optional)
include(GNUInstallDirs)
install(TARGETS dotchart RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
