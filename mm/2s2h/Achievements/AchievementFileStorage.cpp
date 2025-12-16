// Local includes
#include "AchievementFileStorage.h"
#include "StaticData/Types.h"

// Standard library
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <string>
#include <functional>
#include <unordered_map>

// Third-party
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

// Ship/libultraship
#include <ship/Context.h>

namespace Achievements {

namespace FileStorage {

using json = nlohmann::json;

constexpr const char* ACHIEVEMENTS_FILE_NAME = "2S2HAchievementsData.json";
constexpr uint32_t CURRENT_FILE_VERSION = 1;
constexpr const char* FILE_TYPE = "2S2H_ACHIEVEMENTS";

void AchievementFileStorage_Migration_1(json& j);

const std::unordered_map<uint32_t, std::function<void(json&)>> migrations = {
    { 0, AchievementFileStorage_Migration_1 },
};

int MigrateAchievementsFile(json& j) {
    try {
        int version = j.value("version", 0);

        if (version > (int)CURRENT_FILE_VERSION) {
            SPDLOG_ERROR("Achievements file version {} is greater than current version {}", version,
                         CURRENT_FILE_VERSION);
            return -1;
        }

        while (version < (int)CURRENT_FILE_VERSION) {
            if (migrations.contains(version)) {
                auto migration = migrations.at(version);
                migration(j);
            } else {
                SPDLOG_ERROR("Missing migration function for version {} to {}", version, version + 1);
                return -1;
            }
            version = j["version"] = version + 1;
        }
        return 0;
    } catch (std::exception& e) {
        SPDLOG_ERROR("Failed to migrate achievements file: {}", e.what());
        return -1;
    } catch (...) {
        SPDLOG_ERROR("Failed to migrate achievements file");
        return -1;
    }
}

void AchievementFileStorage_Migration_1(json& j) {
    // Migration from version 0 to 1: Establishes baseline format (no data transformation)
}

std::string GetAchievementsFilePath() {
    return Ship::Context::GetPathRelativeToAppDirectory(ACHIEVEMENTS_FILE_NAME);
}

void InitializeAchievementsFile() {
    std::string filePath = GetAchievementsFilePath();

    if (!std::filesystem::exists(filePath)) {
        json initFile =
            json{ { "version", CURRENT_FILE_VERSION },      { "type", FILE_TYPE },
                  { "unlocked", nlohmann::json::array() },  { "events", nlohmann::json::array() },
                  { "counters", nlohmann::json::object() }, { "unlockTimestamps", nlohmann::json::object() } };

        std::ofstream file(filePath);
        if (file.is_open()) {
            file << std::setw(4) << initFile << std::endl;
            if (!file.good()) {
                SPDLOG_ERROR("Failed to write achievements file: {}", filePath);
                return;
            }
            file.close();
        } else {
            SPDLOG_ERROR("Failed to open achievements file for writing: {}", filePath);
        }
    }
}

void LoadAchievementsData(AchievementData& data) {
    std::string filePath = GetAchievementsFilePath();
    json fileData;

    std::ifstream inputFile(filePath);
    if (!inputFile.is_open()) {
        SPDLOG_WARN("Achievements file not found, using defaults: {}", filePath);
        data.unlocked.clear();
        data.events.clear();
        data.counters.clear();
        data.unlockTimestamps.clear();
        return;
    }

    try {
        inputFile >> fileData;
        inputFile.close();

        // Validate file type
        if (!fileData.contains("type") || fileData["type"] != FILE_TYPE) {
            SPDLOG_WARN("Invalid achievements file type, using defaults");
            data.unlocked.clear();
            data.events.clear();
            data.counters.clear();
            data.unlockTimestamps.clear();
            return;
        }

        // Validate and migrate version
        int fileVersion = fileData.value("version", 0);
        if (fileVersion > (int)CURRENT_FILE_VERSION) {
            SPDLOG_ERROR("Achievements file version {} is greater than current version {}, using defaults", fileVersion,
                         CURRENT_FILE_VERSION);
            data.unlocked.clear();
            data.events.clear();
            data.counters.clear();
            data.unlockTimestamps.clear();
            return;
        }
        if (MigrateAchievementsFile(fileData) != 0) {
            SPDLOG_WARN("Failed to migrate achievements file, using defaults");
            data.unlocked.clear();
            data.events.clear();
            data.counters.clear();
            data.unlockTimestamps.clear();
            return;
        }

        // Save migrated file back to disk
        if (fileVersion < (int)CURRENT_FILE_VERSION) {
            try {
                std::ofstream outputFile(filePath);
                if (outputFile.is_open()) {
                    outputFile << std::setw(4) << fileData << std::endl;
                    if (!outputFile.good()) {
                        SPDLOG_WARN("Failed to save migrated achievements file: {}", filePath);
                    }
                    outputFile.close();
                }
            } catch (...) { SPDLOG_WARN("Failed to save migrated achievements file: {}", filePath); }
        }

        // Load unlocked array
        if (fileData.contains("unlocked") && fileData["unlocked"].is_array()) {
            data.unlocked = fileData["unlocked"].get<std::vector<bool>>();
        } else {
            data.unlocked.clear();
        }

        // Load events array
        if (fileData.contains("events") && fileData["events"].is_array()) {
            data.events = fileData["events"].get<std::vector<bool>>();
        } else {
            data.events.clear();
        }

        // Load counters
        if (fileData.contains("counters") && fileData["counters"].is_object()) {
            data.counters.clear();
            for (auto& [key, value] : fileData["counters"].items()) {
                if (value.is_number_unsigned()) {
                    data.counters[key] = value.get<uint32_t>();
                }
            }
        } else {
            data.counters.clear();
        }

        // Load unlock timestamps
        if (fileData.contains("unlockTimestamps") && fileData["unlockTimestamps"].is_object()) {
            data.unlockTimestamps.clear();
            for (auto& [key, value] : fileData["unlockTimestamps"].items()) {
                try {
                    uint32_t achievementId = static_cast<uint32_t>(std::stoul(key));
                    if (achievementId >= static_cast<uint32_t>(AchievementId::ACHIEVEMENT_ID_MAX)) {
                        SPDLOG_WARN("Achievement ID out of range in unlockTimestamps: {}", achievementId);
                        continue;
                    }
                    if (value.is_array()) {
                        std::vector<uint64_t> timestamps;
                        for (auto& ts : value) {
                            if (ts.is_number_unsigned()) {
                                timestamps.push_back(ts.get<uint64_t>());
                            }
                        }
                        data.unlockTimestamps[achievementId] = timestamps;
                    }
                } catch (const std::invalid_argument&) {
                    SPDLOG_WARN("Invalid achievement ID in unlockTimestamps: {}", key);
                    continue;
                } catch (const std::out_of_range&) {
                    SPDLOG_WARN("Achievement ID out of range in unlockTimestamps: {}", key);
                    continue;
                }
            }
        } else {
            data.unlockTimestamps.clear();
        }

    } catch (const nlohmann::json::exception& e) {
        SPDLOG_ERROR("Failed to parse achievements file: {}", e.what());
        data.unlocked.clear();
        data.events.clear();
        data.counters.clear();
        data.unlockTimestamps.clear();
    } catch (...) {
        SPDLOG_ERROR("Failed to load achievements file");
        data.unlocked.clear();
        data.events.clear();
        data.counters.clear();
        data.unlockTimestamps.clear();
    }
}

void SaveAchievementsData(const AchievementData& data) {
    std::string filePath = GetAchievementsFilePath();
    std::filesystem::path filePathObj(filePath);
    auto parentDir = filePathObj.parent_path();

    // Create parent directory if it doesn't exist
    if (!parentDir.empty() && !std::filesystem::exists(parentDir)) {
        try {
            std::filesystem::create_directories(parentDir);
        } catch (const std::filesystem::filesystem_error& e) {
            SPDLOG_ERROR("Failed to create parent directory for achievements file: {}", e.what());
            return;
        }
    }

    json fileData = json{ { "version", CURRENT_FILE_VERSION }, { "type", FILE_TYPE },
                          { "unlocked", data.unlocked },       { "events", data.events },
                          { "counters", data.counters },       { "unlockTimestamps", nlohmann::json::object() } };

    // Convert unlockTimestamps map to JSON object
    for (const auto& [achievementId, timestamps] : data.unlockTimestamps) {
        fileData["unlockTimestamps"][std::to_string(achievementId)] = timestamps;
    }

    try {
        std::ofstream outputFile(filePath);
        if (!outputFile.is_open()) {
            SPDLOG_ERROR("Failed to open achievements file for writing: {}", filePath);
            return;
        }
        outputFile << std::setw(4) << fileData << std::endl;
        if (!outputFile.good()) {
            SPDLOG_ERROR("Failed to write achievements file: {}", filePath);
            return;
        }
        outputFile.close();
    } catch (...) { SPDLOG_ERROR("Failed to save achievements file: {}", filePath); }
}

} // namespace FileStorage

} // namespace Achievements
