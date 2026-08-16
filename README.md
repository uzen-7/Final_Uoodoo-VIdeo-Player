# UooDoo

A C++ video-library prototype.

## What it includes
 - A console version and a native Qt desktop version

## Project Documentation
- [ROOT_README.md](ROOT_README.md) — overview of the whole project
- [src/README.md](src/README.md) — explanation of the core implementation
- [tests/README.md](tests/README.md) — guide to the test suite
- [build/README.md](build/README.md) — explanation of build outputs
- [PROJECT_EXPLANATION.md](PROJECT_EXPLANATION.md) — detailed explanation of what was built, what was used, and how it works

## Build

```bash
g++ -std=c++17 -I src src/main.cpp src/LibraryManager.cpp -o build/uoodoo_app
```

## Run
2. Run the app normally: 
./build/uoodoo_qt

1. set a player for one-off:
UOODOO_PLAYER=vlc ./build/uoodoo_qt


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
g++ -std=c++17 -I src tests/library_manager_test.cpp src/LibraryManager.cpp -o build/uoodoo_test
./build/uoodoo_test
```