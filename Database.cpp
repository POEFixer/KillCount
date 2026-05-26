#include "Database.h"
#include "sqlite3.h"
#include <filesystem>
#include <Windows.h>

bool KillDatabase::Open(const std::filesystem::path& pluginDir) {
    namespace fs = std::filesystem;
    fs::path dataDir = pluginDir / "data";
    if (!fs::exists(dataDir)) {
        fs::create_directories(dataDir);
    }

    fs::path dbPath = dataDir / "killcount.db";
    // sqlite3_open expects a UTF-8 filename on Windows. Build it from the
    // wide form rather than path::string() (which uses the default C locale
    // and throws on non-ASCII).
    std::wstring wide = dbPath.wstring();
    int needed = ::WideCharToMultiByte(
        CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()),
        nullptr, 0, nullptr, nullptr);
    std::string dbPathUtf8(static_cast<size_t>(needed), '\0');
    ::WideCharToMultiByte(
        CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()),
        dbPathUtf8.data(), needed, nullptr, nullptr);

    int rc = sqlite3_open(dbPathUtf8.c_str(), &m_DB);
    if (rc != SQLITE_OK) {
        m_DB = nullptr;
        return false;
    }

    CreateTables();
    return true;
}

void KillDatabase::Close() {
    if (m_DB) {
        sqlite3_close(m_DB);
        m_DB = nullptr;
    }
    m_CurrentSessionId = -1;
}

void KillDatabase::CreateTables() {
    if (!m_DB) return;

    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS lifetime_stats (
            id INTEGER PRIMARY KEY CHECK (id = 1),
            normal_kills INTEGER DEFAULT 0,
            magic_kills INTEGER DEFAULT 0,
            rare_kills INTEGER DEFAULT 0,
            unique_kills INTEGER DEFAULT 0,
            unknown_kills INTEGER DEFAULT 0,
            magic_chests INTEGER DEFAULT 0,
            rare_chests INTEGER DEFAULT 0,
            expedition_chests INTEGER DEFAULT 0,
            breach_chests INTEGER DEFAULT 0,
            strongboxes INTEGER DEFAULT 0,
            player_deaths INTEGER DEFAULT 0,
            last_updated TEXT DEFAULT (datetime('now'))
        );

        CREATE TABLE IF NOT EXISTS session_log (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            area_name TEXT NOT NULL,
            area_hash TEXT NOT NULL,
            area_level INTEGER DEFAULT 0,
            started_at TEXT DEFAULT (datetime('now')),
            ended_at TEXT,
            normal_kills INTEGER DEFAULT 0,
            magic_kills INTEGER DEFAULT 0,
            rare_kills INTEGER DEFAULT 0,
            unique_kills INTEGER DEFAULT 0,
            unknown_kills INTEGER DEFAULT 0,
            magic_chests INTEGER DEFAULT 0,
            rare_chests INTEGER DEFAULT 0,
            expedition_chests INTEGER DEFAULT 0,
            breach_chests INTEGER DEFAULT 0,
            strongboxes INTEGER DEFAULT 0,
            player_deaths INTEGER DEFAULT 0
        );

        INSERT OR IGNORE INTO lifetime_stats (id) VALUES (1);
    )";

    ExecuteSQL(sql);
}

void KillDatabase::ExecuteSQL(const char* sql) {
    if (!m_DB) return;
    char* errMsg = nullptr;
    sqlite3_exec(m_DB, sql, nullptr, nullptr, &errMsg);
    if (errMsg) {
        sqlite3_free(errMsg);
    }
}

MapCounters KillDatabase::LoadLifetimeStats() {
    MapCounters c;
    if (!m_DB) return c;

    const char* sql = "SELECT normal_kills, magic_kills, rare_kills, unique_kills, unknown_kills, "
                      "magic_chests, rare_chests, expedition_chests, breach_chests, strongboxes, "
                      "player_deaths FROM lifetime_stats WHERE id = 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_DB, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            c.NormalKills     = sqlite3_column_int(stmt, 0);
            c.MagicKills      = sqlite3_column_int(stmt, 1);
            c.RareKills       = sqlite3_column_int(stmt, 2);
            c.UniqueKills     = sqlite3_column_int(stmt, 3);
            c.UnknownKills    = sqlite3_column_int(stmt, 4);
            c.MagicChests     = sqlite3_column_int(stmt, 5);
            c.RareChests      = sqlite3_column_int(stmt, 6);
            c.ExpeditionChests = sqlite3_column_int(stmt, 7);
            c.BreachChests    = sqlite3_column_int(stmt, 8);
            c.Strongboxes     = sqlite3_column_int(stmt, 9);
            c.PlayerDeaths    = sqlite3_column_int(stmt, 10);
        }
        sqlite3_finalize(stmt);
    }

    return c;
}

void KillDatabase::SaveLifetimeStats(const MapCounters& c) {
    if (!m_DB) return;

    const char* sql = "UPDATE lifetime_stats SET "
                      "normal_kills=?, magic_kills=?, rare_kills=?, unique_kills=?, unknown_kills=?, "
                      "magic_chests=?, rare_chests=?, expedition_chests=?, breach_chests=?, strongboxes=?, "
                      "player_deaths=?, last_updated=datetime('now') WHERE id=1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_DB, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, c.NormalKills);
        sqlite3_bind_int(stmt, 2, c.MagicKills);
        sqlite3_bind_int(stmt, 3, c.RareKills);
        sqlite3_bind_int(stmt, 4, c.UniqueKills);
        sqlite3_bind_int(stmt, 5, c.UnknownKills);
        sqlite3_bind_int(stmt, 6, c.MagicChests);
        sqlite3_bind_int(stmt, 7, c.RareChests);
        sqlite3_bind_int(stmt, 8, c.ExpeditionChests);
        sqlite3_bind_int(stmt, 9, c.BreachChests);
        sqlite3_bind_int(stmt, 10, c.Strongboxes);
        sqlite3_bind_int(stmt, 11, c.PlayerDeaths);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

void KillDatabase::StartSession(const std::string& areaName, const std::string& areaHash, int areaLevel) {
    if (!m_DB) return;

    const char* sql = "INSERT INTO session_log (area_name, area_hash, area_level) VALUES (?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_DB, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, areaName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, areaHash.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, areaLevel);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    m_CurrentSessionId = sqlite3_last_insert_rowid(m_DB);
}

void KillDatabase::EndSession(const MapCounters& c) {
    if (!m_DB || m_CurrentSessionId < 0) return;

    const char* sql = "UPDATE session_log SET "
                      "ended_at=datetime('now'), "
                      "normal_kills=?, magic_kills=?, rare_kills=?, unique_kills=?, unknown_kills=?, "
                      "magic_chests=?, rare_chests=?, expedition_chests=?, breach_chests=?, strongboxes=?, "
                      "player_deaths=? WHERE id=?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_DB, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, c.NormalKills);
        sqlite3_bind_int(stmt, 2, c.MagicKills);
        sqlite3_bind_int(stmt, 3, c.RareKills);
        sqlite3_bind_int(stmt, 4, c.UniqueKills);
        sqlite3_bind_int(stmt, 5, c.UnknownKills);
        sqlite3_bind_int(stmt, 6, c.MagicChests);
        sqlite3_bind_int(stmt, 7, c.RareChests);
        sqlite3_bind_int(stmt, 8, c.ExpeditionChests);
        sqlite3_bind_int(stmt, 9, c.BreachChests);
        sqlite3_bind_int(stmt, 10, c.Strongboxes);
        sqlite3_bind_int(stmt, 11, c.PlayerDeaths);
        sqlite3_bind_int64(stmt, 12, m_CurrentSessionId);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    m_CurrentSessionId = -1;
}

void KillDatabase::ResetLifetimeStats() {
    if (!m_DB) return;
    ExecuteSQL("UPDATE lifetime_stats SET normal_kills=0, magic_kills=0, rare_kills=0, "
               "unique_kills=0, unknown_kills=0, magic_chests=0, rare_chests=0, "
               "expedition_chests=0, breach_chests=0, strongboxes=0, player_deaths=0, "
               "last_updated=datetime('now') WHERE id=1;");
}
