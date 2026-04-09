#include "KillCount.h"
#include <fstream>
#include <filesystem>
#include <string>

using namespace PluginSDK;

// ============================================================================
// IPlugin implementation
// ============================================================================

void KillCountPlugin::SetPluginDirectory(const char* dir) {
    m_Directory = dir;
}

void KillCountPlugin::SetContext(PluginContext* ctx) {
    m_Context = ctx;
    if (m_Context && m_Context->ImGuiContext) {
        ImGui::SetCurrentContext(static_cast<ImGuiContext*>(m_Context->ImGuiContext));
    }
}

void KillCountPlugin::OnEnable(bool /*isGameOpened*/) {
    LoadSettings();

    if (m_Database.Open(m_Directory)) {
        auto lifetime = m_Database.LoadLifetimeStats();
        m_Tracker.SetLifetimeCounters(lifetime);
    }

    // Initialize renderer position from saved settings
    m_Renderer.SetPosition(m_Settings.PosX, m_Settings.PosY);

    if (m_Context) {
        m_Context->Log("Info", "KillCount plugin enabled");
    }
}

void KillCountPlugin::OnDisable() {
    FlushSession();
    m_Database.SaveLifetimeStats(m_Tracker.GetLifetimeCounters());
    m_Database.Close();
    m_IconAtlas.Release();
    m_Initialized = false;

    if (m_Context) {
        m_Context->Log("Info", "KillCount plugin disabled");
    }
}

void KillCountPlugin::InitializeOnce() {
    if (m_Initialized) return;
    if (!m_Context || !m_Context->D3DDevice) return;

    m_IconAtlas.Initialize(m_Context->D3DDevice);
    m_Renderer.Initialize(&m_IconAtlas);
    m_Initialized = true;
}

void KillCountPlugin::FlushSession() {
    if (m_SessionActive) {
        m_Database.EndSession(m_Tracker.GetPreviousMapCounters());
        m_SessionActive = false;
    }
    m_Database.SaveLifetimeStats(m_Tracker.GetLifetimeCounters());
}

void KillCountPlugin::DrawUI() {
    if (!m_Context) return;
    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(m_Context->ImGuiContext));

    InitializeOnce();

    auto snapshot = m_Context->GetSnapshot();
    if (!snapshot || !snapshot->IsAttached) return;

    bool inGame = (snapshot->CurrentState == GameStateTypes::InGameState);
    if (!inGame) return;

    // Update tracker
    m_Tracker.Update(snapshot);

    // Handle area change — flush old session, start new one
    if (m_Tracker.ConsumeAreaChanged()) {
        FlushSession();

        // Start new session
        m_Database.StartSession(
            m_Tracker.GetCurrentAreaName(),
            m_Tracker.GetCurrentAreaHash(),
            m_Tracker.GetCurrentAreaLevel());
        m_SessionActive = true;
    }

    // Render overlay
    bool menuVisible = m_Context->IsMenuVisible ? m_Context->IsMenuVisible() : true;
    m_Renderer.Render(
        m_Tracker.GetMapCounters(),
        m_Tracker.GetLifetimeCounters(),
        m_Settings,
        menuVisible);

    // Save position if user dragged the overlay
    if (m_Renderer.ConsumePositionChanged()) {
        m_Settings.PosX = m_Renderer.GetPosX();
        m_Settings.PosY = m_Renderer.GetPosY();
    }
}

void KillCountPlugin::DrawSettings() {
    // --- Overlay ---
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.4f, 1.0f), "Overlay");
    ImGui::Separator();
    ImGui::Checkbox("Show Overlay", &m_Settings.ShowOverlay);
    ImGui::Checkbox("Overlay Mode (render on game)", &m_Settings.WantsOverlayMode);
    ImGui::SliderFloat("Opacity", &m_Settings.WindowAlpha, 0.0f, 1.0f, "%.2f");
    ImGui::Spacing();

    // --- Display Mode ---
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.4f, 1.0f), "Counters");
    ImGui::Separator();
    ImGui::Checkbox("Current Map", &m_Settings.ShowMapCounters);
    ImGui::SameLine();
    ImGui::Checkbox("Lifetime Total", &m_Settings.ShowTotalCounters);
    ImGui::Spacing();

    // --- Monsters ---
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.4f, 1.0f), "Monsters");
    ImGui::Separator();
    ImGui::Checkbox("Show Monsters", &m_Settings.ShowMonsters);
    if (m_Settings.ShowMonsters) {
        ImGui::Indent();
        ImGui::Checkbox("Normal",  &m_Settings.ShowNormal);
        ImGui::SameLine();
        ImGui::Checkbox("Magic",   &m_Settings.ShowMagic);
        ImGui::SameLine();
        ImGui::Checkbox("Rare",    &m_Settings.ShowRare);
        ImGui::Checkbox("Unique",  &m_Settings.ShowUnique);
        ImGui::SameLine();
        ImGui::Checkbox("Unknown", &m_Settings.ShowUnknown);
        ImGui::Unindent();
    }
    ImGui::Spacing();

    // --- Chests ---
    ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Chests");
    ImGui::Separator();
    ImGui::Checkbox("Show Chests", &m_Settings.ShowChests);
    if (m_Settings.ShowChests) {
        ImGui::Indent();
        ImGui::Checkbox("Magic##C",     &m_Settings.ShowMagicChest);
        ImGui::SameLine();
        ImGui::Checkbox("Rare##C",      &m_Settings.ShowRareChest);
        ImGui::SameLine();
        ImGui::Checkbox("Expedition",   &m_Settings.ShowExpeditionChest);
        ImGui::Checkbox("Breach",       &m_Settings.ShowBreachChest);
        ImGui::SameLine();
        ImGui::Checkbox("Strongbox##C", &m_Settings.ShowStrongbox);
        ImGui::Unindent();
    }
    ImGui::Spacing();

    // --- Deaths ---
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Other");
    ImGui::Separator();
    ImGui::Checkbox("Show Deaths", &m_Settings.ShowDeaths);
    ImGui::Spacing();

    // --- Reset ---
    ImGui::Separator();
    if (ImGui::Button("Reset Map Counter")) {
        m_Tracker.ResetMapCounters();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Lifetime Stats")) {
        m_Tracker.ResetLifetimeCounters();
        m_Database.ResetLifetimeStats();
    }
}

void KillCountPlugin::SaveSettings() {
    namespace fs = std::filesystem;
    fs::path configDir = fs::path(m_Directory) / "config";
    if (!fs::exists(configDir)) {
        fs::create_directories(configDir);
    }

    std::ofstream file(configDir / "settings.txt");
    if (!file.is_open()) return;

    file << "ShowOverlay=" << (m_Settings.ShowOverlay ? 1 : 0) << "\n";
    file << "WantsOverlayMode=" << (m_Settings.WantsOverlayMode ? 1 : 0) << "\n";
    file << "WindowAlpha=" << m_Settings.WindowAlpha << "\n";
    file << "ShowMapCounters=" << (m_Settings.ShowMapCounters ? 1 : 0) << "\n";
    file << "ShowTotalCounters=" << (m_Settings.ShowTotalCounters ? 1 : 0) << "\n";
    file << "ShowMonsters=" << (m_Settings.ShowMonsters ? 1 : 0) << "\n";
    file << "ShowChests=" << (m_Settings.ShowChests ? 1 : 0) << "\n";
    file << "ShowDeaths=" << (m_Settings.ShowDeaths ? 1 : 0) << "\n";
    file << "ShowNormal=" << (m_Settings.ShowNormal ? 1 : 0) << "\n";
    file << "ShowMagic=" << (m_Settings.ShowMagic ? 1 : 0) << "\n";
    file << "ShowRare=" << (m_Settings.ShowRare ? 1 : 0) << "\n";
    file << "ShowUnique=" << (m_Settings.ShowUnique ? 1 : 0) << "\n";
    file << "ShowUnknown=" << (m_Settings.ShowUnknown ? 1 : 0) << "\n";
    file << "ShowMagicChest=" << (m_Settings.ShowMagicChest ? 1 : 0) << "\n";
    file << "ShowRareChest=" << (m_Settings.ShowRareChest ? 1 : 0) << "\n";
    file << "ShowExpeditionChest=" << (m_Settings.ShowExpeditionChest ? 1 : 0) << "\n";
    file << "ShowBreachChest=" << (m_Settings.ShowBreachChest ? 1 : 0) << "\n";
    file << "ShowStrongbox=" << (m_Settings.ShowStrongbox ? 1 : 0) << "\n";
    file << "PosX=" << m_Settings.PosX << "\n";
    file << "PosY=" << m_Settings.PosY << "\n";

    // Also save lifetime stats to DB
    m_Database.SaveLifetimeStats(m_Tracker.GetLifetimeCounters());
}

void KillCountPlugin::LoadSettings() {
    namespace fs = std::filesystem;
    fs::path settingsPath = fs::path(m_Directory) / "config" / "settings.txt";
    if (!fs::exists(settingsPath)) return;

    std::ifstream file(settingsPath);
    std::string line;
    while (std::getline(file, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        if (key == "ShowOverlay")          m_Settings.ShowOverlay = (val == "1");
        else if (key == "WantsOverlayMode") m_Settings.WantsOverlayMode = (val == "1");
        else if (key == "WindowAlpha")     m_Settings.WindowAlpha = std::stof(val);
        else if (key == "ShowMapCounters") m_Settings.ShowMapCounters = (val == "1");
        else if (key == "ShowTotalCounters") m_Settings.ShowTotalCounters = (val == "1");
        else if (key == "ShowMonsters")    m_Settings.ShowMonsters = (val == "1");
        else if (key == "ShowChests")      m_Settings.ShowChests = (val == "1");
        else if (key == "ShowDeaths")      m_Settings.ShowDeaths = (val == "1");
        else if (key == "ShowNormal")      m_Settings.ShowNormal = (val == "1");
        else if (key == "ShowMagic")       m_Settings.ShowMagic = (val == "1");
        else if (key == "ShowRare")        m_Settings.ShowRare = (val == "1");
        else if (key == "ShowUnique")      m_Settings.ShowUnique = (val == "1");
        else if (key == "ShowUnknown")     m_Settings.ShowUnknown = (val == "1");
        else if (key == "ShowMagicChest")  m_Settings.ShowMagicChest = (val == "1");
        else if (key == "ShowRareChest")   m_Settings.ShowRareChest = (val == "1");
        else if (key == "ShowExpeditionChest") m_Settings.ShowExpeditionChest = (val == "1");
        else if (key == "ShowBreachChest") m_Settings.ShowBreachChest = (val == "1");
        else if (key == "ShowStrongbox")   m_Settings.ShowStrongbox = (val == "1");
        else if (key == "PosX")            m_Settings.PosX = std::stof(val);
        else if (key == "PosY")            m_Settings.PosY = std::stof(val);
    }
}

// ============================================================================
// Factory exports
// ============================================================================

extern "C" PLUGIN_API IPlugin* CreatePlugin() {
    return new KillCountPlugin();
}

extern "C" PLUGIN_API void DestroyPlugin(IPlugin* plugin) {
    delete plugin;
}
