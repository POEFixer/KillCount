#pragma once

#include "sdk/PluginSDK.h"
#include "KillTracker.h"
#include "IconAtlas.h"
#include "OverlayRenderer.h"
#include "Database.h"
#include "DisplaySettings.h"

class KillCountPlugin : public PluginSDK::Plugin {
public:
    void OnEnable(bool isGameAttached) override;
    void OnDisable() override;
    void DrawSettings() override;
    void DrawUI() override;
    void SaveSettings() override;
    const char* GetName() const override { return "Kill Counter"; }
    bool WantsOverlay() const override {
        return m_Settings.WantsOverlayMode && m_Settings.ShowOverlay;
    }

private:
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
