#ifndef ACHIEVEMENTS_H
#define ACHIEVEMENTS_H

// Local includes
#include "StaticData/Registry.h"

// Standard library
#include <cstdint>
#include <vector>

// Ship/libultraship
#include <libultraship/bridge/consolevariablebridge.h>

extern "C" {
#include "variables.h"
}

namespace Achievements {

void Init();
void RegisterAchievementTracker();

bool IsUnlocked(AchievementId achievementId);
bool IsEventTriggered(AchievementEvent achievementEventId);

void GetProgress(AchievementId achievementId, uint32_t& current, uint32_t& max);

// Read-only query functions for viewing achievements (bypass IS_ACHIEVEMENTS check)
bool IsUnlockedReadOnly(AchievementId achievementId);
bool IsEventTriggeredReadOnly(AchievementEvent achievementEventId);
void GetProgressReadOnly(AchievementId achievementId, uint32_t& current, uint32_t& max);
uint32_t GetEventCounterReadOnly(AchievementEvent achievementEventId);

void TriggerEvent(AchievementEvent achievementEventId, bool fromEditor = false);
void SetEventCounter(AchievementEvent achievementEventId, uint32_t count, bool fromEditor = false);
void EnableAchievements();
void Lock(AchievementId achievementId);
void ResetEvent(AchievementEvent achievementEventId);
void QueueEvent(AchievementEvent achievementEventId);
void ProcessQueuedEvents();

} // namespace Achievements

#define IS_ACHIEVEMENTS (CVarGetInteger("gEnhancements.Achievements.Enabled", 0))

#define QUEUE_ACHIEVEMENT(eventId)             \
    do {                                       \
        if (IS_ACHIEVEMENTS) {                 \
            Achievements::QueueEvent(eventId); \
        }                                      \
    } while (0)

#define IS_ACH_TRIGGERED(eventId) Achievements::IsEventTriggered(eventId)
#define IS_ACH_UNLOCKED(id) Achievements::IsUnlocked(id)

#endif // ACHIEVEMENTS_H
