# UooDoo Project Explanation

This document explains what was built, what tools were used, and how the project works in detail.

## What was done

The project was implemented as a C++ video library system with:

- a core C++ library manager
- a console-based app
- a native Qt desktop application
- a test suite for the core logic

The goal was to convert the proposal idea into a practical and runnable C++ application.

## Recent updates — Integrated VideoPlayer

The Qt GUI was extended with an internal `VideoPlayer` implemented in `src/VideoPlayer.h` and `src/VideoPlayer.cpp`. This replaces launching external media apps and provides consistent in-app playback controls.

Implementation highlights:

- Uses Qt6 `QMediaPlayer` + `QVideoWidget` for playback and rendering.
- Custom `LocalSeekBar` widget provides ticks, a draggable handle, and percent-based seeking.
- Exposed API: `load(path)`, `play()`, `pause()`, `stop()`, `seek(ms)`, `toggleFullScreen()`; signals for position/duration/progress are emitted.
- Fullscreen behavior: controls overlay auto-hides after 5s and reappears on mouse/keyboard. Controls are reparented into the video widget when entering fullscreen so they remain visible above the native fullscreen window.
- Keyboard shortcuts implemented for common operations (Space, F, N, P, Left/Right).
- Destructor and lifecycle fixes applied to avoid emitting signals during teardown.

## What was used

### C++
C++ is used for the core logic in:

- src/LibraryManager.h
- src/LibraryManager.cpp
- src/main.cpp

Responsibilities:
- storing information about videos
- adding and removing videos
- marking videos as favorites
- saving and loading the video list

### Qt
The native desktop UI is implemented in:

- uoodoo_qt.cpp

The CMake configuration was updated to link `Qt6::Widgets`, `Qt6::Multimedia`, and `Qt6::MultimediaWidgets`. Ensure Qt6 is installed on your system when building the GUI.

It provides a modern desktop interface with search, add/remove, favorites, and playback launch.

### CMake
CMake organizes the build (see `CMakeLists.txt`) and builds the console app, the Qt app, and tests.

### Tests
Automated tests live in `tests/library_manager_test.cpp` and verify the library manager behavior.

## How the project works

The project stores a local list of videos; each entry contains a name, path, and favorite flag. The UI layers (console or Qt) use the library manager API to present and modify the data.

## How to build and run

Build the Qt GUI:

```bash
cmake -S . -B build
cmake --build build --target uoodoo_qt
./build/uoodoo_qt
```

Build and run the console app:

```bash
g++ -std=c++17 -I src src/main.cpp src/LibraryManager.cpp -o build/uoodoo_app
./build/uoodoo_app
```

Run tests:

```bash
g++ -std=c++17 -I src tests/library_manager_test.cpp src/LibraryManager.cpp -o build/uoodoo_test
./build/uoodoo_test
```

This project now focuses on C++ and Qt; Python artifacts have been removed.
