cmake_minimum_required(VERSION 3.16)
project(SoundWaveVisualizer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set SFML path
set(SFML_DIR "C:/SFML-3.0.2/lib/cmake/SFML")

find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

set(SOURCES
    src/main.cpp
    src/visualizer/bar_visualizer.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE SFML::Graphics SFML::Window SFML::System)

# Windows Audio Libraries (Needed for MiniAudio)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ole32
        winmm
        dwmapi
    )
endif()

# 6. COPY DLLs (Automatic for Windows)
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SFML::Graphics>
        $<TARGET_FILE:SFML::Window>
        $<TARGET_FILE:SFML::System>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()