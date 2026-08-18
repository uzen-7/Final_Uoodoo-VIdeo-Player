# Tests Guide

This folder contains the automated test for the library manager.

## Purpose

The test verifies that the core video-library logic is working correctly.

## Test File

### library_manager_test.cpp
This test creates a temporary video file and a temporary metadata file, then checks that:

- a video can be added successfully
- the video appears in the library
- toggling favorite works
- the stored video can be retrieved correctly

## Why This Matters

The tests protect the core features of UooDoo from breaking as the project grows. They confirm that the data model and file-handling logic behave as intended.

## How It Works

1. A temporary sample video file is created.
2. A LibraryManager instance is created with a temporary metadata path.
3. The test adds a video entry and checks the result.
4. The favorite flag is toggled and verified.
5. The test finishes and reports success if everything passes.

## Running Tests (CMake)

If you built the project with CMake, run the tests via CTest from the build directory:

```bash
ctest --test-dir build
```

Or build and run the single test executable directly:

```bash
cmake --build build --target uoodoo_test
./build/uoodoo_test
```
