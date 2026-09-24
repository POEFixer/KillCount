#pragma once

#include "IconAtlas.h"
#include "KillTracker.h"
#include "DisplaySettings.h"

class OverlayRenderer {
public:
    void Initialize(IconAtlas* atlas);
    void Render(const MapCounters& mapCounters, const MapCounters& lifetimeCounters,
                const DisplaySettings& settings, bool isMenuVisible);

    // Position management — call after Render() to check if user dragged the window
    bool ConsumePositionChanged();
    float GetPosX() const { return m_PosX; }
    float GetPosY() const { return m_PosY; }
    void SetPosition(float x, float y) { m_PosX = x; m_PosY = y; }

private:
    IconAtlas* m_Atlas = nullptr;
    float m_PosX = 100.0f;
    float m_PosY = 100.0f;
    bool m_PositionChanged = false;

    void RenderCounters(const MapCounters& counters, const DisplaySettings& settings);
    void RenderRow(const char* label, const std::string& iconName, int count);
};
