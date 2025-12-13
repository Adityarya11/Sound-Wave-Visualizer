# SoundWave Visualizer

- **Platform:** Windows 10/11 (x64)
- **Language:** C++17

## Project Overview

SoundWave Visualizer is a high-performance, desktop-native audio visualization utility designed for the Windows operating system. Unlike traditional media players that only visualize the files they play, SoundWave utilizes the Windows Audio Session API (WASAPI) Loopback feature to intercept and visualize global system audio in real-time.

Whether the audio source is a web browser (YouTube, Spotify), a game, or a local media player, SoundWave renders a responsive, frequency-based visualization overlay. The application is designed to be unobtrusive, featuring a borderless, transparent window architecture that integrates seamlessly with the user's desktop environment.

## Key Features

### Core Functionality

- **System-Wide Audio Capture:** Utilizes low-latency WASAPI loopback to capture audio from any running application without requiring virtual audio cables.
- **Real-Time FFT Analysis:** Implements Fast Fourier Transform algorithms to convert time-domain audio signals into frequency-domain data for accurate spectrum visualization.
- **High-Performance Rendering:** Built on SFML 3.0 for hardware-accelerated 2D graphics, ensuring 60 FPS performance with minimal CPU overhead.
- **Desktop Overlay Architecture:** Functions as a draggable, "always-on-top" utility designed to sit at the bottom of the screen or on a secondary monitor.

### Configuration & Customization

- **JSON-Based Configuration:** All application settings (window size, position, audio sensitivity, color schemes) are stored in a local `config.json` file, allowing for easy backup and manual editing.
- **Dynamic Resizing:** The visualization engine automatically adapts to the configured window dimensions.

## Technical Architecture

The application is engineered as a modular C++ system comprising three primary distinct subsystems: Data Acquisition, Signal Processing, and Rendering.

### System Data Flow

```text
[Windows OS Audio Mixer]
       |
       v
[MiniAudio Backend (WASAPI Loopback)] --> Capture Thread
       |
       v
[Raw PCM Data Buffer]
       |
       v
[KissFFT Processor] --> Converts Time Domain to Frequency Domain
       |
       v
[Normalization & Smoothing Engine] --> Applies Linear Interpolation (Lerp)
       |
       v
[SFML Render Engine] --> Draws Geometry to Window
```

### Technology Stack

- **Language:** C++17
- **Build System:** CMake (3.16+)
- **Graphics Library:** SFML 3.0.0 (Simple and Fast Multimedia Library)
- **Audio Engine:** MiniAudio (Single-header audio playback and capture library)
- **Signal Processing:** KissFFT (Lightweight Fast Fourier Transform library)
- **Serialization:** nlohmann/json (JSON for Modern C++)

## Project Directory Structure

The codebase adheres to a standard CMake project layout to ensure maintainability and separation of concerns.

```text
SoundWaveVisualizer/
├── assets/                  # Runtime resources (shaders, fonts, icons)
├── build/                   # Compilation output directory (excluded from version control)
├── config.json              # User configuration file
├── CMakeLists.txt           # Main build configuration
├── include/                 # Header-only third-party libraries
│   ├── kissfft/             # FFT implementation
│   ├── nlohmann/            # JSON parser
│   └── miniaudio.h          # Audio engine header
├── src/                     # Source code
│   ├── main.cpp             # Application entry point
│   ├── audio/               # Audio capture and processing logic
│   │   ├── AudioCapture.cpp
│   │   ├── AudioCapture.hpp
│   │   ├── FftProcessor.cpp
│   │   └── FftProcessor.hpp
│   ├── core/                # System management
│   │   ├── App.cpp          # Application lifecycle
│   │   ├── Config.cpp       # JSON loader/saver
│   │   └── Window.cpp       # SFML window wrapper
│   └── visualizer/          # Rendering logic
│       ├── VisualizerBase.hpp
│       └── BarVisualizer.cpp
└── README.md                # Documentation
```

## Build Instructions

### Prerequisites

1.  **Operating System:** Windows 10 or Windows 11 (64-bit).
2.  **Compiler:** Microsoft Visual Studio 2022 (MSVC) with "Desktop development with C++" workload installed.
3.  **Build Tool:** CMake 3.16 or higher.
4.  **Git:** For version control.

### Installation Steps

1.  **Clone the Repository**

    ```powershell
    git clone https://github.com/YourUsername/SoundWaveVisualizer.git
    cd SoundWaveVisualizer
    ```

2.  **Create Build Directory**

    ```powershell
    mkdir build
    cd build
    ```

3.  **Configure Project**
    Run CMake to generate the Visual Studio solution files. Ensure you target the x64 architecture.

    ```powershell
    cmake ..
    ```

4.  **Compile**
    Build the project in Debug or Release mode.

    ```powershell
    cmake --build . --config Release
    ```

5.  **Run**
    Navigate to the Release folder and execute the binary.

    ```powershell
    ./Release/SoundWaveVisualizer.exe
    ```

## Configuration Guide

The application behavior is controlled via `config.json` located in the root directory.

**Example Configuration:**

```json
{
  "window": {
    "width": 800,
    "height": 200,
    "position_x": -1,
    "position_y": -1,
    "always_on_top": true
  },
  "audio": {
    "sensitivity": 1.5,
    "smoothing": 0.8,
    "fft_size": 1024
  },
  "visualizer": {
    "mode": "bars",
    "color_primary": [255, 0, 0, 255],
    "bar_count": 64,
    "bar_spacing": 2
  }
}
```

_Note: Setting `position_x` or `position_y` to -1 will center the window automatically._

## Development Roadmap

### Phase 1: Foundation (Current Status)

- Establish CMake build system with MSVC support.
- Integrate SFML 3.0 for window management.
- Implement WASAPI Loopback capture via MiniAudio.
- Basic FFT signal processing.
- Render basic frequency bars.

### Phase 2: Visualization Enhancements

- **Visualizer Modes:** Implement interchangeable renderers (Waveform, Circular Spectrum, Spectrogram).
- **Color Themes:** Support for gradient maps and dynamic color shifting based on amplitude.
- **Post-Processing:** Implementation of GLSL shaders for "Bloom" and "Glow" effects.
- **Beat Detection:** Algorithms to detect transient peaks (bass kicks) to trigger visual pulses.

### Phase 3: User Experience & Polish

- **UI Overlay:** Integration of Dear ImGui for a runtime settings menu (accessible via context menu).
- **System Tray Integration:** Minimize to tray functionality.
- **Performance Optimization:** Multi-threading the audio capture loop to separate it from the rendering thread.
- **Multi-Monitor Support:** Logic to detect and snap to specific display coordinates.

## Troubleshooting

**Issue: CMake cannot find Visual Studio.**

- _Solution:_ Ensure you have installed the "Desktop development with C++" workload via the Visual Studio Installer.

**Issue: "Unresolved External Symbol" errors.**

- _Solution:_ Ensure you are linking against the 64-bit (x64) version of SFML. The 32-bit (x86) libraries are incompatible with the default 64-bit MSVC compiler settings.

**Issue: Application starts but shows a black screen.**

- _Solution:_ Play audio on your system. The visualizer requires active audio output to generate data. Ensure your default output device is set correctly in Windows Sound Settings.
