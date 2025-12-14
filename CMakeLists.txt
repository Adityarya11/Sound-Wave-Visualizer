cmake_minimum_required(VERSION 3.16)
project(SoundWaveVisualizer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# === IMPORTANT: Ensure this path points to your SFML installation ===
set(SFML_DIR "C:/SFML-3.0.2/lib/cmake/SFML")

find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

# Define all source files
# CRITICAL FIX: Added 'include/kissfft/kiss_fft.c' to this list
set(SOURCES
    src/main.cpp
    src/audio/audio_capture.cpp
    src/audio/fft_processor.cpp
    src/visualizer/bar_visualizer.cpp
    include/kissfft/kiss_fft.c 
)

add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories so code can find "miniaudio.h" and "kiss_fft.h"
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE SFML::Graphics SFML::Window SFML::System)

# Windows specific libraries (Required for Audio Loopback)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ole32
        winmm
        dwmapi
    )
endif()

# Auto-copy SFML DLLs to the build folder
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SFML::Graphics>
        $<TARGET_FILE:SFML::Window>
        $<TARGET_FILE:SFML::System>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()