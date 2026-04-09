#pragma once

#include "KillTracker.h"
#include <string>
#include <cstdint>

// Forward declare — full header included only in Database.cpp
struct sqlite3;

class KillDatabase {
public:
    bool Open(const std::string& pluginDir);
    void Close();

    MapCounters LoadLifetimeStats();
    void SaveLifetimeStats(const MapCounters& counters);

    void StartSession(const std::string& areaName, const std::string& areaHash, int areaLevel);
    void EndSession(const MapCounters& mapCounters);

    void ResetLifetimeStats();

private:
    sqlite3* m_DB = nullptr;
    int64_t m_CurrentSessionId = -1;

    void ExecuteSQL(const char* sql);
    void CreateTables();
};
