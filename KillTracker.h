#pragma once

#include "sdk/PluginSDK.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

// Per-map / lifetime statistics
struct MapCounters {
    int NormalKills = 0;
    int MagicKills = 0;
    int RareKills = 0;
    int UniqueKills = 0;
    int UnknownKills = 0;

    int MagicChests = 0;
    int RareChests = 0;
    int ExpeditionChests = 0;
    int BreachChests = 0;
    int Strongboxes = 0;

    int PlayerDeaths = 0;

    int TotalMonsterKills() const {
        return NormalKills + MagicKills + RareKills + UniqueKills + UnknownKills;
    }
    int TotalChestOpens() const {
        return MagicChests + RareChests + ExpeditionChests + BreachChests + Strongboxes;
    }
};

struct TrackedEntity {
    uint32_t Id = 0;
    int Rarity = 0;
    PluginSDK::EntityType    Type    = PluginSDK::EntityType::Unidentified;
    PluginSDK::EntitySubtype Subtype = PluginSDK::EntitySubtype::Unidentified;
    PluginSDK::NearbyZone    Zone    = PluginSDK::NearbyZone::None;
    bool WasChestClosed = false; // Only used for chests
};

class KillTracker {
public:
    void Update(const PluginSDK::Snapshot& snapshot);

    const MapCounters& GetMapCounters() const { return m_MapCounters; }
    const MapCounters& GetLifetimeCounters() const { return m_LifetimeCounters; }

    void SetLifetimeCounters(const MapCounters& counters) { m_LifetimeCounters = counters; }

    // Returns true if area changed since last call (caller can flush DB)
    bool ConsumeAreaChanged();

    // Get current area info for session logging
    const std::string& GetCurrentAreaName() const { return m_CurrentAreaName; }
    const std::string& GetCurrentAreaHash() const { return m_CurrentAreaHash; }
    int GetCurrentAreaLevel() const { return m_CurrentAreaLevel; }

    // Get the map counters that were active before the pending reset
    const MapCounters& GetPreviousMapCounters() const { return m_PreviousMapCounters; }

    void ResetMapCounters();
    void ResetLifetimeCounters();

private:
    MapCounters m_MapCounters;
    MapCounters m_LifetimeCounters;
    MapCounters m_PreviousMapCounters; // Saved before pending reset

    uint64_t m_LastAreaChangeCounter = 0;
    bool m_PendingReset = false;
    bool m_AreaChanged = false;

    std::string m_CurrentAreaName;
    std::string m_CurrentAreaHash;
    int m_CurrentAreaLevel = 0;

    std::unordered_map<uint32_t, TrackedEntity> m_PrevEntities;
    bool m_PlayerWasAlive = true;

    void DetectKills(const std::vector<PluginSDK::Entity>& entities);
    void DetectChestOpens(const std::vector<PluginSDK::Entity>& entities);
    void DetectPlayerDeath(const PluginSDK::Vitals& vitals);
    void IncrementKill(int rarity);
    void IncrementChest(PluginSDK::EntitySubtype subtype);
};
