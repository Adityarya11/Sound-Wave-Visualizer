# Sound Visualiser

My first project in C++.

## Overview

Inspired by the NCS-style circular music visualiser and by Hyprland's aurora
gradients, this is a desktop-native audio visualisation overlay for Windows.
It uses the WASAPI loopback feature to intercept and visualise global system
audio in real time — no virtual audio cable required.

It floats above everything, you can put it anywhere on screen and resize it,
and it fades away on its own when the music stops.

![Image 1](images/1.png)

### Core functionality

- **System-wide audio capture** — low-latency WASAPI loopback picks up any
  running application. Tested with YouTube, Spotify and games.
- **Real-time FFT analysis** — KissFFT converts the signal to the frequency
  domain, then maps it onto logarithmically spaced bands, so bass, mids and
  treble each get a fair share of the display.
- **Beat detection** — bass energy is tracked against its own rolling average,
  so the orb pulses on transients rather than just on loudness.
- **True per-pixel transparency** — the overlay composites properly against
  whatever is behind it, with a soft glow rather than hard cut-out edges.
- **Auto-fade** — when audio stops it fades out and drops to 15 FPS, so it
  never nags at you from an idle desktop.

### Two modes

| Mode | Look |
| --- | --- |
| **Orb** (default) | Circular aurora visualiser. Radius breathes with the beat, the rim spikes with the spectrum, the palette drifts through violet, magenta, cyan and mint. Rendered entirely in a GLSL shader for soft bloom. |
| **Bars** | Linear spectrum, mirrored around the centre — bass in the middle, pitch rising outward — with a reflection underneath. |

### System data flow

```text
[Windows OS Audio Mixer]
       |
       v
[MiniAudio (WASAPI Loopback)] --> capture thread, ring buffer
       |
       v
[KissFFT] --> magnitudes --> logarithmic bands + level + beat
       |
       v
[GLSL aurora shader / vertex geometry]
       |
       v
[UpdateLayeredWindow] --> per-pixel alpha on the desktop
```

## Install

Requires [MSYS2](https://www.msys2.org/). Install the toolchain and SFML once:

```powershell
pacman -S mingw-w64-x86_64-sfml
```

Then, from the repository root:

```powershell
.\install.ps1
```

That builds a Release binary, copies it to
`%LOCALAPPDATA%\Programs\Visualizer`, and adds that folder to your user PATH.
Open a new terminal and run:

```powershell
visualizer
```

The executable is fully static — around 5.5 MB, with no SFML or MinGW DLLs to
carry around. `.\install.ps1 -Uninstall` reverses everything.

### Building without installing

```powershell
cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_C_COMPILER=C:/msys64/mingw64/bin/gcc.exe `
  -DCMAKE_CXX_COMPILER=C:/msys64/mingw64/bin/g++.exe `
  -DCMAKE_MAKE_PROGRAM=C:/msys64/mingw64/bin/mingw32-make.exe `
  -DCMAKE_PREFIX_PATH=C:/msys64/mingw64
cmake --build build-mingw -j 4
```

## Controls

The overlay never takes keyboard focus, so the hotkeys are global — they work
whatever you happen to be using at the time.

| Hotkey | Action |
| --- | --- |
| `Ctrl+Alt+V` | Lock / unlock for moving |
| `Ctrl+Alt+B` | Switch between orb and bars |
| `Ctrl+Alt+Up` / `Down` | Bigger / smaller |
| `Ctrl+Alt+Q` | Quit |

While unlocked: drag to move it, scroll to resize, `Esc` or right-click to lock
again. `visualizer --help` shows all of this in a dialog.

Locked is the normal state, and while locked the overlay is completely
click-through — you can work straight through it.

Position, size and mode are remembered in `%APPDATA%\Visualizer\config.json`.

## Tweaking the colours

`aurora.frag` sits next to the installed executable. Edit it and restart —
the loose file overrides the copy compiled into the binary, so you can retune
the palette without a rebuild. The five colour stops near the top of
`palette()` are the place to start; `src/core/aurora.hpp` holds the matching
list used by bar mode, so change both to keep the two consistent.

<<<<<<< HEAD
<!-- <img src="https://github.com/user-attachments/assets/9400ee43-8f33-4258-ab64-2da620de4e90"
     width="1366"
     height="600"
     alt="Video"> -->

[Watch the video](https://github.com/user-attachments/assets/9400ee43-8f33-4258-ab64-2da620de4e90)
=======
## Notes

Motivated by [Tsoding (UI in C++)](https://www.youtube.com/watch?v=SRgLA8X5N_4).
>>>>>>> d28eaf1 (Add PATH installer and rewrite the docs)

[Video of an earlier version](images/Visualiser.mp4)

If you find this fun and cool then thanks!
