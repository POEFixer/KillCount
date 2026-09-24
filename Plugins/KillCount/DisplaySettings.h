#pragma once

struct DisplaySettings {
    // Display mode
    bool ShowMapCounters = true;
    bool ShowTotalCounters = false;

    // Section toggles
    bool ShowMonsters = true;
    bool ShowChests = true;
    bool ShowDeaths = true;

    // Monster rarity toggles
    bool ShowNormal = true;
    bool ShowMagic = true;
    bool ShowRare = true;
    bool ShowUnique = true;
    bool ShowUnknown = false;

    // Chest type toggles
    bool ShowMagicChest = true;
    bool ShowRareChest = true;
    bool ShowExpeditionChest = true;
    bool ShowBreachChest = true;
    bool ShowStrongbox = true;

    // Overlay appearance
    float WindowAlpha = 0.85f;
    bool ShowOverlay = true;
    bool WantsOverlayMode = true;

    // Overlay position (saved/restored)
    float PosX = 100.0f;
    float PosY = 100.0f;
};
