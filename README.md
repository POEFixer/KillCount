# KillCount — Kill/Chest/Death Tracking Plugin

A comprehensive tracking overlay for POE2Fixer that counts monster kills, chest opens, and player deaths with per-map and lifetime statistics. All data is persisted in a local SQLite3 database.

## What It Does

KillCount monitors game entities in real-time and tracks three categories of events:

### Monster Kill Tracking
Counts killed monsters by rarity: **Normal**, **Magic**, **Rare**, **Unique**, and **Unknown**. Uses disappearance-based detection — monitors entities within two configurable proximity zones (InnerCircle ~60 grid units, OuterCircle ~120 grid units) and registers a kill when a monster disappears from the entity list.

### Chest Tracking
Counts opened chests by type: **Magic**, **Rare**, **Expedition**, **Breach**, and **Strongbox**. Detects state transitions (closed -> opened) on chest entities to avoid double-counting.

### Death Counter
Tracks player deaths by monitoring HP transitions. When the player's health drops to zero, a death is registered.

## How It Works

### Per-Map Statistics
Counters automatically reset when entering a new area. The previous area's counters remain visible until the first event occurs in the new area, so you can review your last map results.

### Lifetime Statistics
All data is stored in a SQLite3 database (`data/killcount.db` in the plugin directory). The database tracks session history per area including area name, hash, level, timestamps, and all counters. Lifetime totals aggregate across all sessions.

### Overlay Display
- **Draggable** when the POE2Fixer menu is active (a "Drag to reposition" hint appears)
- **Non-interactive** and title-bar-free when the menu is hidden — won't interfere with gameplay
- **Configurable opacity** from 0% to 100%
- **Tab bar** for switching between Map (current area) and Total (lifetime) views — both can be enabled simultaneously
- **Color-coded sections**: orange headers for Monsters, light blue for Chests, red for Deaths
- **Icon atlas**: displays entity type icons from the shared radar sprite sheet with fallback icons for all categories

### Settings
- Toggle individual counter categories (5 monster types + 5 chest types + deaths)
- Adjust overlay opacity
- Choose overlay mode
- Reset buttons for map statistics and lifetime totals

## Build

**Requirements:** Visual Studio 2022 (MSVC v143), Windows SDK 10.0, C++20

Open `KillCount.sln` and build **Release | x64**.

Or from command line:
```
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" KillCount.sln -p:Configuration=Release -p:Platform=x64
```

Output: `bin\Release\KillCount.dll`

## Dependencies

All dependencies are bundled — no external installs needed:
- **SQLite3** (`lib/`) — database engine, compiled as C
- **nlohmann/json** (`nlohmann/`) — JSON parser for icon atlas metadata
- **stb_image** (`imgui/stb_image.h`) — PNG image loading for icon atlas
- **IconsFontAwesome6** (`imgui/IconsFontAwesome6.h`) — font icon codepoints
- **ImGui** (`imgui/`) — immediate-mode GUI library
- **Plugin SDK** (`sdk/`) — POE2Fixer plugin interface headers

## Install

Copy `KillCount.dll` to `Plugins/KillCount/` in your POE2Fixer directory, then enable it in the Plugins tab. The database file is created automatically on first run.
