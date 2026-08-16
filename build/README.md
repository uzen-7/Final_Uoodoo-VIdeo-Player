# Build Output Guide

This folder contains the generated build artifacts for the project.

## What Goes Here

When the project is compiled, this directory receives the executable files and intermediate build files created by CMake or the compiler.

## Typical Files

- uoodoo_app: the console version of the app
- uoodoo_test: the automated test executable
- uoodoo_qt: the graphical Qt desktop application
- CMake-generated files for configuration and build steps

## Purpose

The build folder keeps compiled outputs separate from the source files so the project remains organized and easier to maintain.

## Notes

Do not edit files in this folder manually unless you are debugging a build issue. The build system usually regenerates them automatically.
