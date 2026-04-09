# KillCount Plugin — Standalone Distribution

Kill/chest/death tracking overlay plugin for POE2Fixer. Tracks monster kills by rarity, chest opens by type, and player deaths with per-map and lifetime statistics. Data is persisted in a SQLite3 database. Uses an icon atlas from the host application's radar resources for visual display.

## Build

Requirements: Visual Studio 2022 (MSVC v143), C++20, x64.

Open `KillCount.sln` and build **Release | x64**.

Or from command line:
```
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" KillCount.sln -p:Configuration=Release -p:Platform=x64
```

Output: `bin\Release\KillCount.dll`

## Dependencies

All dependencies are bundled:
- **SQLite3** (`lib/`) — database engine, compiled as C
- **ImGui** (`imgui/`) — immediate-mode GUI library
- **nlohmann/json** (`nlohmann/`) — JSON parser for icon atlas configuration
- **stb_image** (`imgui/stb_image.h`) — PNG image loading for icon atlas
- **Plugin SDK** (`sdk/`) — POE2Fixer plugin interface headers

## Installation

Copy the built `KillCount.dll` into your POE2Fixer `Plugins/KillCount/` directory.

## Features

- Monster kill tracking by rarity (Normal, Magic, Rare, Unique)
- Chest open tracking by type (Magic, Rare, Expedition, Breach, Strongbox)
- Player death counting
- Per-map and lifetime statistics with tab switching
- Draggable overlay with configurable opacity
- SQLite3-backed persistent storage with session logging
- Icon atlas integration for visual entity icons
