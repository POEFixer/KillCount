#include "OverlayRenderer.h"
#include "imgui/IconsFontAwesome6.h"

void OverlayRenderer::Initialize(IconAtlas* atlas) {
    m_Atlas = atlas;
}

bool OverlayRenderer::ConsumePositionChanged() {
    if (m_PositionChanged) {
        m_PositionChanged = false;
        return true;
    }
    return false;
}

void OverlayRenderer::RenderRow(const char* label, const std::string& iconName, int count) {
    float iconSize = ImGui::GetTextLineHeight() + 2.0f;
    float windowWidth = ImGui::GetContentRegionAvail().x;

    if (m_Atlas && m_Atlas->IsLoaded()) {
        const auto& icon = m_Atlas->GetIcon(iconName);
        if (icon.Valid) {
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            float offsetY = (ImGui::GetTextLineHeight() - iconSize) * 0.5f;
            ImGui::GetWindowDrawList()->AddImage(
                m_Atlas->GetTextureID(),
                ImVec2(cursorPos.x, cursorPos.y + offsetY),
                ImVec2(cursorPos.x + iconSize, cursorPos.y + offsetY + iconSize),
                icon.UV0, icon.UV1);
            ImGui::Dummy(ImVec2(iconSize, ImGui::GetTextLineHeight()));
            ImGui::SameLine();
        }
    }

    ImGui::Text("%s", label);

    // Right-align the count
    char countStr[32];
    snprintf(countStr, sizeof(countStr), "%d", count);
    float countWidth = ImGui::CalcTextSize(countStr).x;
    ImGui::SameLine(windowWidth - countWidth);
    ImGui::Text("%s", countStr);
}

void OverlayRenderer::RenderCounters(const MapCounters& counters, const DisplaySettings& settings) {
    if (settings.ShowMonsters) {
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.4f, 1.0f), "Monsters");
        ImGui::Separator();

        if (settings.ShowNormal)  RenderRow("Normal",  "Normal Monster",  counters.NormalKills);
        if (settings.ShowMagic)   RenderRow("Magic",   "Magic Monster",   counters.MagicKills);
        if (settings.ShowRare)    RenderRow("Rare",    "Rare Monster",    counters.RareKills);
        if (settings.ShowUnique)  RenderRow("Unique",  "Unique Monster",  counters.UniqueKills);
        if (settings.ShowUnknown) RenderRow("Unknown", "Unknown Monster", counters.UnknownKills);

        // Total right-aligned
        char totalStr[32];
        snprintf(totalStr, sizeof(totalStr), "Total: %d", counters.TotalMonsterKills());
        float totalWidth = ImGui::CalcTextSize(totalStr).x;
        float windowWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + windowWidth - totalWidth);
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", totalStr);
        ImGui::Spacing();
    }

    if (settings.ShowChests) {
        ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Chests");
        ImGui::Separator();

        if (settings.ShowMagicChest)      RenderRow("Magic",      "Magic Chests",              counters.MagicChests);
        if (settings.ShowRareChest)       RenderRow("Rare",       "Rare Chests",               counters.RareChests);
        if (settings.ShowExpeditionChest) RenderRow("Expedition", "Generic Expedition Chests",  counters.ExpeditionChests);
        if (settings.ShowBreachChest)     RenderRow("Breach",     "Breach Chest",              counters.BreachChests);
        if (settings.ShowStrongbox)       RenderRow("Strongbox",  "Strongbox",                 counters.Strongboxes);

        char totalStr[32];
        snprintf(totalStr, sizeof(totalStr), "Total: %d", counters.TotalChestOpens());
        float totalWidth = ImGui::CalcTextSize(totalStr).x;
        float windowWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + windowWidth - totalWidth);
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", totalStr);
        ImGui::Spacing();
    }

    if (settings.ShowDeaths) {
        ImGui::Separator();
        RenderRow("Deaths", "Death", counters.PlayerDeaths);
    }
}

void OverlayRenderer::Render(const MapCounters& mapCounters, const MapCounters& lifetimeCounters,
                              const DisplaySettings& settings, bool isMenuVisible) {
    if (!settings.ShowOverlay) return;

    if (isMenuVisible) {
        // === DRAGGABLE MODE: ImGui window with drag hint (like AutoPot overlay) ===
        ImGui::SetNextWindowPos(ImVec2(m_PosX, m_PosY), ImGuiCond_Appearing);
        ImGui::SetNextWindowBgAlpha(settings.WindowAlpha);
        ImGui::SetNextWindowSizeConstraints(ImVec2(150, 0), ImVec2(220, 600));

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing;

        ImGui::Begin("##KillCountOverlay", nullptr, flags);

        // Drag hint
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
            ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT " Drag to reposition");
        ImGui::Spacing();

        // Tab bar for Map / Total
        if (settings.ShowMapCounters && settings.ShowTotalCounters) {
            if (ImGui::BeginTabBar("##KCTabs")) {
                if (ImGui::BeginTabItem("Map")) {
                    RenderCounters(mapCounters, settings);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Total")) {
                    RenderCounters(lifetimeCounters, settings);
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        } else if (settings.ShowMapCounters) {
            RenderCounters(mapCounters, settings);
        } else if (settings.ShowTotalCounters) {
            RenderCounters(lifetimeCounters, settings);
        }

        // Save position if dragged
        ImVec2 pos = ImGui::GetWindowPos();
        if (pos.x != m_PosX || pos.y != m_PosY) {
            m_PosX = pos.x;
            m_PosY = pos.y;
            m_PositionChanged = true;
        }

        ImGui::End();
    }
    else {
        // === NON-INTERACTIVE MODE: static overlay, no drag, no title ===
        ImGui::SetNextWindowPos(ImVec2(m_PosX, m_PosY));
        ImGui::SetNextWindowBgAlpha(settings.WindowAlpha);
        ImGui::SetNextWindowSizeConstraints(ImVec2(150, 0), ImVec2(220, 600));

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoInputs;

        ImGui::Begin("##KillCountOverlay", nullptr, flags);

        // Content only (no drag hint, no tabs when non-interactive)
        if (settings.ShowMapCounters) {
            RenderCounters(mapCounters, settings);
        } else if (settings.ShowTotalCounters) {
            RenderCounters(lifetimeCounters, settings);
        }

        ImGui::End();
    }
}
