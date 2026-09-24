#pragma once

#include "KillTracker.h"
#include <string>
#include <cstdint>
#include <filesystem>

// Forward declare — full header included only in Database.cpp
struct sqlite3;

class KillDatabase {
public:
    // pluginDir comes from Plugin::DirectoryPath() — already a wide-correct
    // fs::path. Passing the UTF-8 std::string from Directory() through
    // fs::path on Windows would corrupt non-ASCII chars (system ANSI
    // codepage interpretation creates mojibake folders).
    bool Open(const std::filesystem::path& pluginDir);
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
