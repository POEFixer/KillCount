> **Retired (2026-10-22).** This plugin is no longer maintained or distributed by POE2Fixer. It does not load in POE2Fixer 364 and newer. Install current plugins from POE2Fixer -> Plugins -> Plugin Manager.

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

## Build from source

This repository is the plugin folder from the POEFixer source tree (`Plugins/KillCount/`) together with the host files it compiles against, kept at the same relative paths:

- `POEFixer/plugin_sdk/` - the POEFixer Plugin SDK v6 headers (`PluginAbi.h`, `PluginSDK.h`)
- `POEFixer/imgui/` - the Dear ImGui sources the DLL compiles against

1. Install Visual Studio 2022 with the "Desktop development with C++" workload (MSVC v143, Windows 10/11 SDK).
2. Open `KillCount.sln`, select **Release | x64** and build the solution.
3. The plugin is written to `x64\Release\Plugins\KillCount\KillCount.dll`.

## Install

Copy `KillCount.dll` into `Plugins\KillCount\` next to the POEFixer executable (create the folder if needed), start POEFixer and enable the plugin in the Plugins tab.
