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