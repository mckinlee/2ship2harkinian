#ifndef ACHIEVEMENT_FILE_STORAGE_H
#define ACHIEVEMENT_FILE_STORAGE_H

// Local includes
#include "StaticData/Types.h"

// Standard library
#include <cstdint>
#include <map>
#include <string>
#include <vector>

// Third-party
#include <nlohmann/json.hpp>

namespace Achievements {

namespace FileStorage {

struct AchievementData {
    std::vector<bool> unlocked;
    std::vector<bool> events;
    std::map<std::string, uint32_t> counters;
    std::map<uint32_t, std::vector<uint64_t>> unlockTimestamps;
};

void InitializeAchievementsFile();
void LoadAchievementsData(AchievementData& data);
void SaveAchievementsData(const AchievementData& data);

// Migration system
int MigrateAchievementsFile(nlohmann::json& j);

} // namespace FileStorage

} // namespace Achievements

#endif // ACHIEVEMENT_FILE_STORAGE_H
