cmake_minimum_required(VERSION 3.16)
project(Visualizer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# Produce one self-contained visualizer.exe with no DLLs beside it.
# Turn OFF if you only have shared SFML; the install script will then copy DLLs.
option(VISUALIZER_STATIC "Link SFML and the GCC runtime statically" ON)

if(VISUALIZER_STATIC)
    set(SFML_STATIC_LIBRARIES ON)
    # CMake resolves .dll.a before .a, which would drag in shared freetype/png/etc.
    # Restricting the suffix list forces the static archives that MSYS2 ships.
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
endif()

find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

# Bake assets/aurora.frag into a header so the exe carries its own shader and
# can be dropped anywhere on PATH. Re-run configure when the shader changes.
file(READ ${CMAKE_SOURCE_DIR}/assets/aurora.frag AURORA_FRAG_SOURCE)
configure_file(${CMAKE_SOURCE_DIR}/cmake/aurora_frag.hpp.in
               ${CMAKE_BINARY_DIR}/generated/aurora_frag.hpp @ONLY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
             ${CMAKE_SOURCE_DIR}/assets/aurora.frag)

add_executable(visualizer WIN32
    src/main.cpp
    src/core/app.cpp
    src/core/config.cpp
    src/core/overlay.cpp
    src/audio/audio_capture.cpp
    src/audio/fft_processor.cpp
    src/visualizer/bar_visualizer.cpp
    src/visualizer/circle_visualizer.cpp
    include/kissfft/kiss_fft.c
)

target_include_directories(visualizer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(visualizer PRIVATE SFML::Graphics SFML::Window SFML::System)

if(WIN32)
    # Gives visualizer.exe its own icon in Explorer and the task switcher.
    # Regenerate assets/icon.ico with: python tools/make_icon.py
    enable_language(RC)
    target_sources(visualizer PRIVATE ${CMAKE_SOURCE_DIR}/assets/visualizer.rc)
    set_source_files_properties(${CMAKE_SOURCE_DIR}/assets/visualizer.rc
        PROPERTIES COMPILE_FLAGS "-I${CMAKE_SOURCE_DIR}/assets")

    # ole32/winmm: WASAPI loopback via miniaudio. dwmapi: per-pixel alpha compositing.
    # shell32/user32/advapi32: tray-less overlay plumbing, hotkeys, PATH-friendly config.
    target_link_libraries(visualizer PRIVATE ole32 winmm dwmapi shell32 user32 advapi32)
    target_compile_definitions(visualizer PRIVATE
        WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
endif()

if(VISUALIZER_STATIC AND MINGW)
    # Without this the exe still needs libstdc++-6.dll / libgcc_s_seh-1.dll / libwinpthread-1.dll.
    target_link_options(visualizer PRIVATE
        -static -static-libgcc -static-libstdc++ -mwindows)
endif()

# The shader is compiled into the binary (see src/visualizer/circle_visualizer.cpp).
# A copy is installed next to the exe so it can be hot-swapped without rebuilding.
add_custom_command(TARGET visualizer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/assets/aurora.frag
        $<TARGET_FILE_DIR:visualizer>/aurora.frag)

if(NOT VISUALIZER_STATIC AND WIN32)
    add_custom_command(TARGET visualizer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SFML::Graphics> $<TARGET_FILE:SFML::Window>
            $<TARGET_FILE:SFML::System> $<TARGET_FILE_DIR:visualizer>)
endif()
