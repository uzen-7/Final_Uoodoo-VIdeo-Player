# Source Code Guide

This folder contains the core implementation of the UooDoo project.

## Files

### LibraryManager.h
Defines the data model for video entries and the library-management interface.

- VideoEntry stores the video name, file location, and favorite flag.
- LibraryManager provides methods for adding, removing, listing, and favoriting videos.
- The class also loads and saves metadata from a text file so the app can remember videos between runs.

### LibraryManager.cpp
Contains the working implementation of the library manager.

- addVideo validates a selected file and stores it in the library.
- removeVideo deletes a video from the in-memory list and updates the metadata file.
- toggleFavorite flips the favorite flag for a selected video.
- save writes the library contents to disk in a simple line-based format.
- load reads the saved file when the program starts.

## Working Mechanism

1. The program starts and creates a LibraryManager object.
2. The manager loads any previously saved video metadata.
3. Users can add or remove entries through the app interface.
4. Each change is saved immediately so the library remains persistent.

## Notes

This part of the project represents the backend logic of UooDoo. It is independent from the GUI and can be reused by both console and graphical versions.

### VideoPlayer.h / VideoPlayer.cpp
Implements an integrated video player used by the Qt GUI. Key points:

- `VideoPlayer` is a QWidget-based component that embeds `QVideoWidget` and `QMediaPlayer`.
- API: `load(const QString& path)`, `play()`, `pause()`, `stop()`, `seek(qint64 ms)`, `toggleFullScreen()`.
- Signals: `positionChangedMS(qint64)`, `durationChangedMS(qint64)`, and `progressUpdated(int)` for UI synchronization.
- UI: a local `LocalSeekBar` for seeks, time label showing elapsed/total, playback buttons (Start/Play-Pause/Stop/Prev/Next/Fullscreen), and keyboard shortcuts.
- Fullscreen: controls are reparented into the fullscreen video window so the overlay stays visible and auto-hides after 5s while playing.

See `src/VideoPlayer.cpp` for implementation details and `uoodoo_qt.cpp` for how the player is integrated into the application UI.