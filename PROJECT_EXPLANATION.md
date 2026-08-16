# UooDoo Project Explanation

This document explains what was built, what tools were used, and how the project works in detail.

## What was done

The project was implemented as a C++ video library system with:

- a core C++ library manager
- a console-based app
- a native Qt desktop application
- a test suite for the core logic

The goal was to convert the proposal idea into a practical and runnable C++ application.

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
