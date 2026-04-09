#pragma once

#include "sdk/PluginHelpers.h"
#include "KillTracker.h"
#include "IconAtlas.h"
#include "OverlayRenderer.h"
#include "Database.h"
#include "DisplaySettings.h"

class KillCountPlugin : public IPlugin {
public:
    void SetPluginDirectory(const char* dir) override;
    void SetContext(PluginContext* ctx) override;
    void OnEnable(bool isGameOpened) override;
    void OnDisable() override;
    void DrawSettings() override;
    void DrawUI() override;
    void SaveSettings() override;
    const char* GetName() override { return "Kill Counter"; }
    bool WantsOverlay() override { return m_Settings.WantsOverlayMode && m_Settings.ShowOverlay; }

private:
    PluginContext* m_Context = nullptr;
    std::string m_Directory;

    DisplaySettings m_Settings;
    KillTracker m_Tracker;
    IconAtlas m_IconAtlas;
    OverlayRenderer m_Renderer;
    KillDatabase m_Database;

    bool m_Initialized = false;
    bool m_SessionActive = false;

    void LoadSettings();
    void InitializeOnce();
    void FlushSession();
};
