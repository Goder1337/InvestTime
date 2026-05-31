# StudyWithMe Visual Studio Project

This folder contains the Visual Studio / Qt Widgets project for InvestTime.

For the full bilingual documentation, see the root [README.md](../README.md).

## Quick Start

1. Open `StudyWithMeVS.sln` with Visual Studio 2019 or newer.
2. Select an x64 configuration such as `Debug | x64`.
3. Make sure Qt VS Tools can find your Qt 6 MSVC kit.
4. Press `F5` to build and debug.

## Notes

- User-specific files such as `.vcxproj.user` are intentionally ignored.
- Build outputs under `bin/`, `obj/`, `.vs/`, `Debug/`, and `Release/` are intentionally ignored.
- Large `.flac` audio files are ignored because GitHub rejects files over 100 MB.
