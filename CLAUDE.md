# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A Windows-only desktop audio visualiser: a borderless, always-on-top, per-pixel
transparent overlay that reacts to whatever the system is playing (WASAPI
loopback, no virtual cable). Ships as a single self-contained `visualizer.exe`
intended to live on PATH.

## Toolchain

**MinGW-w64 from MSYS2 only.** There is no Visual Studio on this machine, and
`C:/SFML-3.0.2` is an MSVC build whose `.lib` files MinGW cannot link. SFML
comes from `pacman -S mingw-w64-x86_64-sfml` and lives in `C:/msys64/mingw64`.
If a build suddenly cannot find SFML, that package is the thing to check.

## Commands

```powershell
# Configure (once)
cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_C_COMPILER=C:/msys64/mingw64/bin/gcc.exe `
  -DCMAKE_CXX_COMPILER=C:/msys64/mingw64/bin/g++.exe `
  -DCMAKE_MAKE_PROGRAM=C:/msys64/mingw64/bin/mingw32-make.exe `
  -DCMAKE_PREFIX_PATH=C:/msys64/mingw64

cmake --build build-mingw -j 4      # build
.\install.ps1                       # build + install to PATH as `visualizer`
.\install.ps1 -SkipBuild            # install what is already built
.\install.ps1 -Uninstall            # remove exe and PATH entry
```

There is no test suite. Verification is visual: run the exe, play audio, and
screenshot. `Get-Process visualizer` reports CPU and working set.

**The build fails with `cannot open output file visualizer.exe: Permission
denied` when an instance is still running.** Kill it first
(`Get-Process visualizer | Stop-Process -Force`); `install.ps1` does this
automatically.

Changing `assets/aurora.frag` requires a re-run of CMake configure (the file is
baked into a generated header). A build does this automatically via
`CMAKE_CONFIGURE_DEPENDS`.

## Architecture

Data flows one way, and each stage knows nothing about the next:

```
WASAPI loopback -> AudioCapture (ring buffer, audio thread)
                -> FftProcessor (log bands + level + beat)
                -> VisualState  -> CircleVisualizer / BarVisualizer
                -> Overlay::present()  -> UpdateLayeredWindow
```

- `src/core/app.cpp` owns the loop and is the only place stages meet.
- `src/core/overlay.*` is all the Win32. Nothing else includes `windows.h`
  except `app.cpp` (hotkeys) and `config.cpp` (paths).
- Visualisers receive a `VisualState` by const reference and are pure
  renderers; they never touch audio or config.

### Three decisions that are load-bearing

**Transparency uses `UpdateLayeredWindow`, not DWM.** Both documented DWM
tricks — `DwmEnableBlurBehindWindow` over an empty region, and
`DwmExtendFrameIntoClientArea` with `MARGINS {-1,-1,-1,-1}` — return `S_OK` on
Windows 11 build 26200 and then composite the window **opaque black**. This was
verified with a minimal repro, not assumed. So the render path is: draw with
SFML, `glReadPixels` the RGBA back, hand the DIB to `UpdateLayeredWindow`.
Consequences worth remembering:

- `window.display()` is never called. `Overlay::present()` replaces it, and
  `App::run` paces frames itself because SFML's `setFramerateLimit` lives
  inside `display()`.
- The DIB uses a **positive** `biHeight` so its bottom-up row order matches
  what `glReadPixels` returns; no per-frame flip.
- Colour must be premultiplied. It already is: SFML's alpha blend multiplies
  colour by alpha going into a buffer that `clear(Transparent)` zeroed.
- A layered window passes clicks through every zero-alpha pixel. That is why
  move mode paints a faint full-window wash — without it the window is only
  grabbable where the orb happens to be lit.

**Bands are logarithmic.** `FftProcessor` maps FFT bins onto log-spaced bands
(28 Hz to 16 kHz) and applies a treble tilt. Indexing raw bins directly is what
caused the original "all the bars are bunched on the left" bug: linear bins put
half the display above 10 kHz where music has no energy. Do not reintroduce
direct bin indexing in a visualiser — consume `FftProcessor::bands()`.

**The orb is one shader pass, not geometry.** `CircleVisualizer` draws a single
full-window quad running `assets/aurora.frag`; the spectrum reaches the GPU as
a 64x1 texture. Rotated rectangles cannot produce soft bloom. The shader is
compiled into the binary (generated `aurora_frag.hpp`) *and* copied next to the
exe, where a loose file wins — so colours can be retuned without rebuilding.

### Shader gotchas

Anything driven by a 0..1 angle tears at the wrap, because `noise(0)` and
`noise(1)` are unrelated. Use the unit direction vector `dir` as the angular
coordinate instead. Angular terms must also fade out near the centre (`swirl`),
or `dir` — undefined as `d` approaches 0 — paints a pinwheel of streaks. The
glow is radial but the window is rectangular, so `glow` is tapered to zero
before the edge; without that the overlay reads as a faint grey box.

## Conventions

- Keep `Config` the single source of truth for anything persisted; it round
  trips through `%APPDATA%\Visualizer\config.json` and tolerates corruption by
  falling back to defaults.
- Audio failing is never fatal. `AudioCapture::poll` retries on a cooldown so
  the overlay survives the user switching output device mid-song.
- Global hotkeys are polled with `GetAsyncKeyState`, not `RegisterHotKey`: the
  window is `WS_EX_NOACTIVATE` and receives no keyboard messages at all.
