#include "KillTracker.h"

void KillTracker::Update(const PluginSDK::Snapshot& snapshot) {
    // Detect area change
    if (snapshot.AreaChangeCounter != m_LastAreaChangeCounter) {
        if (m_LastAreaChangeCounter != 0) {
            // Save current map counters before they get reset
            m_PreviousMapCounters = m_MapCounters;
            m_AreaChanged = true;
        }
        m_LastAreaChangeCounter = snapshot.AreaChangeCounter;
        m_PendingReset = true;
        m_PrevEntities.clear();
        m_PlayerWasAlive = true;

        // Update area info
        m_CurrentAreaName = snapshot.CurrentAreaName;
        m_CurrentAreaHash = snapshot.CurrentAreaHash;
        m_CurrentAreaLevel = snapshot.CurrentAreaLevel;
    }

    // Skip tracking in town/hideout
    if (snapshot.IsTown || snapshot.IsHideout) return;

    // Skip if not in game
    if (snapshot.State != PluginSDK::GameState::InGame) return;

    DetectKills(snapshot.Entities);
    DetectChestOpens(snapshot.Entities);
    DetectPlayerDeath(snapshot.Vitals);
}

bool KillTracker::ConsumeAreaChanged() {
    if (m_AreaChanged) {
        m_AreaChanged = false;
        return true;
    }
    return false;
}

void KillTracker::ResetMapCounters() {
    m_MapCounters = {};
    m_PendingReset = false;
}

void KillTracker::ResetLifetimeCounters() {
    m_LifetimeCounters = {};
}

void KillTracker::DetectKills(const std::vector<PluginSDK::Entity>& entities) {
    std::unordered_set<uint32_t> currentMonsterIds;

    // Update tracking map with current monsters
    for (const auto& entity : entities) {
        if (entity.EntityType != PluginSDK::EntityType::Monster) continue;
        if (entity.EntityState == PluginSDK::EntityState::MonsterFriendly) continue;

        currentMonsterIds.insert(entity.Id);

        auto& tracked = m_PrevEntities[entity.Id];
        tracked.Id = entity.Id;
        tracked.Rarity = entity.Rarity;
        tracked.Type = entity.EntityType;
        tracked.Subtype = entity.EntitySubtype;
        tracked.Zone = entity.Zone;
    }

    // Disappearance-based kill detection:
    // Dead entities are filtered OUT of the snapshot (EntityState == Useless).
    // If a monster was previously in InnerCircle or OuterCircle and now disappeared,
    // it was almost certainly killed (not just walked out of range).
    for (auto it = m_PrevEntities.begin(); it != m_PrevEntities.end(); ) {
        if (it->second.Type != PluginSDK::EntityType::Monster) {
            ++it;
            continue;
        }

        if (currentMonsterIds.find(it->first) == currentMonsterIds.end()) {
            // Monster disappeared — count as kill if it was close enough
            if (it->second.Zone == PluginSDK::NearbyZone::InnerCircle ||
                it->second.Zone == PluginSDK::NearbyZone::OuterCircle) {
                IncrementKill(it->second.Rarity);
            }
            it = m_PrevEntities.erase(it);
        } else {
            ++it;
        }
    }
}

void KillTracker::DetectChestOpens(const std::vector<PluginSDK::Entity>& entities) {
    std::unordered_set<uint32_t> currentChestIds;

    for (const auto& entity : entities) {
        if (entity.EntityType != PluginSDK::EntityType::Chest) continue;

        currentChestIds.insert(entity.Id);

        if (!entity.IsChestOpened) {
            // Track closed chest
            auto& tracked = m_PrevEntities[entity.Id];
            tracked.Id = entity.Id;
            tracked.WasChestClosed = true;
            tracked.Rarity = entity.Rarity;
            tracked.Type = entity.EntityType;
            tracked.Subtype = entity.EntitySubtype;
            tracked.Zone = entity.Zone;
            continue;
        }

        // Chest is now opened — check for closed->opened transition
        auto it = m_PrevEntities.find(entity.Id);
        if (it != m_PrevEntities.end() && it->second.WasChestClosed) {
            it->second.WasChestClosed = false;
            it->second.Zone = entity.Zone;
            IncrementChest(entity.EntitySubtype);
        }
    }

    // Also detect chests that disappeared while closed (opened and removed from entity list)
    for (auto it = m_PrevEntities.begin(); it != m_PrevEntities.end(); ) {
        if (it->second.Type != PluginSDK::EntityType::Chest) {
            ++it;
            continue;
        }

        if (currentChestIds.find(it->first) == currentChestIds.end()) {
            // Chest disappeared — if it was closed and nearby, count as opened
            if (it->second.WasChestClosed &&
                (it->second.Zone == PluginSDK::NearbyZone::InnerCircle ||
                 it->second.Zone == PluginSDK::NearbyZone::OuterCircle)) {
                IncrementChest(it->second.Subtype);
            }
            it = m_PrevEntities.erase(it);
        } else {
            ++it;
        }
    }
}

void KillTracker::DetectPlayerDeath(const PluginSDK::Vitals& vitals) {
    if (!vitals.IsValid) return;
    if (vitals.IsTownOrHideout) return;

    bool isAlive = vitals.CurrentHP > 0;

    if (m_PlayerWasAlive && !isAlive) {
        // Apply pending reset for deaths too
        if (m_PendingReset) {
            m_MapCounters = {};
            m_PendingReset = false;
        }
        m_MapCounters.PlayerDeaths++;
        m_LifetimeCounters.PlayerDeaths++;
    }

    m_PlayerWasAlive = isAlive;
}

void KillTracker::IncrementKill(int rarity) {
    if (m_PendingReset) {
        m_MapCounters = {};
        m_PendingReset = false;
    }

    switch (rarity) {
        case 0:  m_MapCounters.NormalKills++;  m_LifetimeCounters.NormalKills++;  break;
        case 1:  m_MapCounters.MagicKills++;   m_LifetimeCounters.MagicKills++;   break;
        case 2:  m_MapCounters.RareKills++;    m_LifetimeCounters.RareKills++;    break;
        case 3:  m_MapCounters.UniqueKills++;  m_LifetimeCounters.UniqueKills++;  break;
        default: m_MapCounters.UnknownKills++; m_LifetimeCounters.UnknownKills++; break;
    }
}

void KillTracker::IncrementChest(PluginSDK::EntitySubtype subtype) {
    if (m_PendingReset) {
        m_MapCounters = {};
        m_PendingReset = false;
    }

    switch (subtype) {
        case PluginSDK::EntitySubtype::ChestMagic:
            m_MapCounters.MagicChests++;  m_LifetimeCounters.MagicChests++;  break;
        case PluginSDK::EntitySubtype::ChestRare:
            m_MapCounters.RareChests++;   m_LifetimeCounters.RareChests++;   break;
        case PluginSDK::EntitySubtype::ExpeditionChest:
            m_MapCounters.ExpeditionChests++; m_LifetimeCounters.ExpeditionChests++; break;
        case PluginSDK::EntitySubtype::BreachChest:
            m_MapCounters.BreachChests++; m_LifetimeCounters.BreachChests++; break;
        case PluginSDK::EntitySubtype::Strongbox:
        case PluginSDK::EntitySubtype::JewellerStrongbox:
        case PluginSDK::EntitySubtype::ResearcherStrongbox:
        case PluginSDK::EntitySubtype::LargeStrongbox:
            m_MapCounters.Strongboxes++;  m_LifetimeCounters.Strongboxes++;  break;
        default:
            break;
    }
}
