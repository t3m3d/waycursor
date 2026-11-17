# xcursor-lite — libXcursor replacement, X11-free.
# Builds libXcursor.so.1 (ABI-compatible with libXcursor 1.2) plus the
# public header and a pkg-config file Hyprland's CMake consumes.
cmake_minimum_required(VERSION 3.16)
project(xcursor-lite VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
add_compile_options(-Wall -Wextra -Wpedantic)

add_library(Xcursor SHARED src/xcursor.c)
target_include_directories(Xcursor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(Xcursor PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION 1
  OUTPUT_NAME Xcursor)

install(TARGETS Xcursor LIBRARY DESTINATION lib)
install(FILES include/X11/Xcursor/Xcursor.h
        DESTINATION include/X11/Xcursor)

# pkg-config file: name must stay "xcursor" so Hyprland's
# pkg_check_modules(... xcursor ...) finds it.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/xcursor.pc.in
               ${CMAKE_CURRENT_BINARY_DIR}/xcursor.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/xcursor.pc
        DESTINATION lib/pkgconfig)

# Smoke test binary (not installed)
add_executable(test-xcursor tests/test_xcursor.c)
target_link_libraries(test-xcursor PRIVATE Xcursor)
