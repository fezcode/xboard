cmake_minimum_required(VERSION 3.20)
project(xboard C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
pkg_check_modules(SDL2_TTF REQUIRED IMPORTED_TARGET SDL2_ttf)

add_library(xboard_core STATIC
  src/app_actions.c
  src/app_shell.c
  src/cli.c
  src/text_buffer.c
  src/controller.c
  src/audio.c
  src/font.c
  src/morse.c
  src/phrases.c
  src/send_input.c
  src/ui_draw.c
  src/mode_dial.c
  src/mode_grid.c
  src/mode_morse.c
)

target_link_libraries(xboard_core PUBLIC
  PkgConfig::SDL2
  PkgConfig::SDL2_TTF
)

if(WIN32)
  target_link_libraries(xboard_core PUBLIC
    xinput9_1_0
    user32
    gdi32
    winmm
  )
endif()

target_include_directories(xboard_core PUBLIC src)
add_executable(xboard src/main.c)
target_link_libraries(xboard PRIVATE xboard_core)
if(WIN32)
  enable_language(RC)
  # Derive Windows metadata from the same version used by the application.
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS src/app_state.h)
  file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/app_state.h" XBOARD_VERSION_LINE REGEX "^#define XBOARD_VERSION ")
  string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" XBOARD_APP_VERSION "${XBOARD_VERSION_LINE}")
  if(NOT XBOARD_APP_VERSION)
    message(FATAL_ERROR "XBOARD_VERSION must be X.Y.Z")
  endif()
  string(REPLACE "." "," XBOARD_VERSION_NUMERIC "${XBOARD_APP_VERSION}")
  configure_file(assets/windows.rc.in windows.rc @ONLY)
  target_sources(xboard PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/windows.rc")
endif()

include(CTest)
if(BUILD_TESTING)
  add_executable(text_buffer_tests tests/text_buffer_tests.c src/text_buffer.c src/morse.c)
  target_include_directories(text_buffer_tests PRIVATE src)
  add_test(NAME text_buffer_tests COMMAND text_buffer_tests)
  add_executable(workflow_tests tests/workflow_tests.c)
  target_link_libraries(workflow_tests PRIVATE xboard_core)
  add_test(NAME workflow_tests COMMAND workflow_tests)
  add_executable(dial_audio_tests tests/dial_audio_tests.c)
  target_link_libraries(dial_audio_tests PRIVATE xboard_core)
  add_test(NAME dial_audio_tests COMMAND dial_audio_tests)
  add_executable(audio_engine_tests tests/audio_engine_tests.c)
  target_link_libraries(audio_engine_tests PRIVATE PkgConfig::SDL2)
  add_test(NAME audio_engine_tests COMMAND audio_engine_tests)
  # Argument parsing has no SDL dependency, so keep this suite standalone.
  add_executable(cli_tests tests/cli_tests.c src/cli.c)
  target_include_directories(cli_tests PRIVATE src)
  add_test(NAME cli_tests COMMAND cli_tests)
endif()
