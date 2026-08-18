# UooDoo

A C++ video-library prototype with a native Qt GUI and integrated video player.

## What it includes
 - A console version and a native Qt desktop version

## Project Documentation
- [ROOT_README.md](ROOT_README.md) — overview of the whole project
- [src/README.md](src/README.md) — explanation of the core implementation
- [tests/README.md](tests/README.md) — guide to the test suite
- [build/README.md](build/README.md) — explanation of build outputs
- [PROJECT_EXPLANATION.md](PROJECT_EXPLANATION.md) — detailed explanation of what was built, what was used, and how it works

## New: Integrated Video Player

The Qt UI now contains an internal `VideoPlayer` (implemented in `src/VideoPlayer.{h,cpp}`) that replaces launching external players. Key features:

- Controls: Start (restart+play), Play/Pause toggle, Stop, Prev, Next, Fullscreen
- Timeline/seek scrubber with tick marks and draggable handle
- Elapsed / total time display
- Keyboard shortcuts: `Space` (play/pause), `F` (fullscreen), `N`/`P` (next/prev), `Left`/`Right` (seek ±5s)
- Fullscreen overlay: controls auto-hide after 5s of playback and reappear on mouse movement or keyboard use

Use the GUI list to select a video and press the Start/Play controls — no external player required.

## Build

```bash
# Quick build (console app)
g++ -std=c++17 -I src src/main.cpp src/LibraryManager.cpp -o build/uoodoo_app

# Recommended: CMake-based build (Qt6 required for GUI)
cmake -S . -B build
cmake --build build --target uoodoo_qt
```

## Run
Run the Qt GUI:

```bash
# From repository root after CMake build
./build/uoodoo_qt
# Or if you built in a separate output folder (cmake -B build_native):
./build_native/uoodoo_qt
```

Note: the project previously supported launching an external player via `UOODOO_PLAYER`, but the preferred method is the integrated `VideoPlayer` in the GUI.


### Console demo
```bash
./build/uoodoo_app
```

### Graphical desktop UI
```bash
cmake -S . -B build
cmake --build build --target uoodoo_qt
./build/uoodoo_qt
```

## Test

```bash
# Build and run the test (simple, direct invocation)
g++ -std=c++17 -I src tests/library_manager_test.cpp src/LibraryManager.cpp -o build/uoodoo_test
./build/uoodoo_test

# Or build via CMake targets (recommended when using the CMake build)
cmake --build build --target uoodoo_test
ctest --test-dir build
```