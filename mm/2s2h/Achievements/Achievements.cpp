// Local includes
#include "Achievements.h"
#include "AchievementIntegration.h"
#include "AchievementFileStorage.h"
#include "StaticData/Registry.h"

// Standard library
#include <cstring>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

// Third-party
#include <spdlog/spdlog.h>

// Ship/libultraship
#include <libultraship/bridge/consolevariablebridge.h>

extern "C" {
#include "functions.h"
#include "variables.h"
}

// 2s2h
#include "2s2h/BenGui/Notification.h"
#include "2s2h/BenPort.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"

namespace Achievements {

namespace {
enum AchievementQueueEventType { QUEUE_ACHIEVEMENT, SHOW_UNLOCK, SHOW_PROGRESS };

struct AchievementQueueEvent {
    AchievementQueueEventType type;
    AchievementEvent achievementEventId;
    AchievementId achievementId;
    uint32_t currentProgress = 0;
    uint32_t maxProgress = 0;
    AchievementEvent triggeringEventId;
};

std::vector<AchievementQueueEvent> achievementQueue;
bool processing = false;
FileStorage::AchievementData achievementData;
static std::mutex achievementDataMutex;
static std::mutex achievementQueueMutex;

inline bool IsValidAchievementId(AchievementId achievementId) {
    const int idIndex = static_cast<int>(achievementId);
    return idIndex >= 0 && idIndex < static_cast<int>(AchievementId::ACHIEVEMENT_ID_MAX);
}

inline bool IsValidEventId(AchievementEvent achievementEventId) {
    const int eventIndex = static_cast<int>(achievementEventId);
    return eventIndex >= 0 && eventIndex < static_cast<int>(AchievementEvent::ACHIEVEMENT_EVENT_MAX);
}

void EnsureVectorsSized(FileStorage::AchievementData& data) {
    if (data.unlocked.size() < static_cast<size_t>(AchievementId::ACHIEVEMENT_ID_MAX)) {
        data.unlocked.resize(static_cast<size_t>(AchievementId::ACHIEVEMENT_ID_MAX), false);
    }
    if (data.events.size() < static_cast<size_t>(AchievementEvent::ACHIEVEMENT_EVENT_MAX)) {
        data.events.resize(static_cast<size_t>(AchievementEvent::ACHIEVEMENT_EVENT_MAX), false);
    }
}

std::string GetEventCounterKey(AchievementEvent eventId) {
    return std::to_string(static_cast<int>(eventId));
}

uint32_t GetEventCounter(AchievementEvent eventId) {
    std::lock_guard<std::mutex> lock(achievementDataMutex);
    EnsureVectorsSized(achievementData);
    std::string key = GetEventCounterKey(eventId);
    auto it = achievementData.counters.find(key);
    return (it != achievementData.counters.end()) ? it->second : 0;
}

uint32_t GetEventCounterUnlocked(AchievementEvent eventId, const FileStorage::AchievementData& data) {
    // Assumes mutex is already held
    std::string key = GetEventCounterKey(eventId);
    auto it = data.counters.find(key);
    return (it != data.counters.end()) ? it->second : 0;
}

void IncrementEventCounter(AchievementEvent eventId) {
    if (!IsValidEventId(eventId)) {
        return;
    }

    FileStorage::AchievementData dataCopy;
    {
        std::lock_guard<std::mutex> lock(achievementDataMutex);
        EnsureVectorsSized(achievementData);
        std::string key = GetEventCounterKey(eventId);
        achievementData.counters[key]++;
        // Also update boolean flag for backward compatibility
        const int eventIndex = static_cast<int>(eventId);
        achievementData.events[eventIndex] = true;
        dataCopy = achievementData;
    }
    FileStorage::SaveAchievementsData(dataCopy);
}

void ResetEventCounter(AchievementEvent eventId) {
    if (!IsValidEventId(eventId)) {
        return;
    }

    FileStorage::AchievementData dataCopy;
    {
        std::lock_guard<std::mutex> lock(achievementDataMutex);
        EnsureVectorsSized(achievementData);
        std::string key = GetEventCounterKey(eventId);
        achievementData.counters[key] = 0;
        // Also update boolean flag for backward compatibility
        const int eventIndex = static_cast<int>(eventId);
        achievementData.events[eventIndex] = false;
        dataCopy = achievementData;
    }
    FileStorage::SaveAchievementsData(dataCopy);
}

void SetEventCounterValue(AchievementEvent eventId, uint32_t count) {
    if (!IsValidEventId(eventId)) {
        return;
    }

    FileStorage::AchievementData dataCopy;
    {
        std::lock_guard<std::mutex> lock(achievementDataMutex);
        EnsureVectorsSized(achievementData);
        std::string key = GetEventCounterKey(eventId);
        achievementData.counters[key] = count;
        // Also update boolean flag for backward compatibility
        const int eventIndex = static_cast<int>(eventId);
        achievementData.events[eventIndex] = (count > 0);
        dataCopy = achievementData;
    }
    FileStorage::SaveAchievementsData(dataCopy);
}

bool IsAchievementComplete(const Achievement* achievement) {
    if (!achievement) {
        return false;
    }

    std::lock_guard<std::mutex> lock(achievementDataMutex);
    EnsureVectorsSized(achievementData);

    for (const AchievementEvent requiredEvent : achievement->requiredEvents) {
        // Get required count (defaults to 1 if not specified)
        uint32_t requiredCount = 1;
        auto it = achievement->eventCounts.find(requiredEvent);
        if (it != achievement->eventCounts.end()) {
            requiredCount = it->second;
        }

        // Get current counter value (using unlocked version since we already hold the lock)
        uint32_t currentCount = GetEventCounterUnlocked(requiredEvent, achievementData);

        // Check if we've met the required count
        if (currentCount < requiredCount) {
            return false;
        }
    }
    return true;
}

void UnlockAchievement(AchievementId achievementId, bool fromEditor) {
    if (!IsValidAchievementId(achievementId)) {
        return;
    }

    const Achievement* achievement = StaticData::GetAchievement(achievementId);
    if (!achievement) {
        return;
    }

    FileStorage::AchievementData dataCopy;
    {
        std::lock_guard<std::mutex> lock(achievementDataMutex);
        const int idIndex = static_cast<int>(achievementId);

        EnsureVectorsSized(achievementData);

        achievementData.unlocked[idIndex] = true;
        uint64_t timestamp = GetUnixTimestamp();
        achievementData.unlockTimestamps[idIndex].push_back(timestamp);
        dataCopy = achievementData;
    }
    FileStorage::SaveAchievementsData(dataCopy);

    if (fromEditor) {
        Notification::EmitAchievement(achievement->iconPath ? achievement->iconPath : "",
                                      std::string(achievement->name), achievement->harbourMastery);
    } else {
        std::lock_guard<std::mutex> lock(achievementQueueMutex);
        achievementQueue.push_back({ SHOW_UNLOCK, AchievementEvent::ACHIEVEMENT_EVENT_MAX, achievementId, 0, 0,
                                     AchievementEvent::ACHIEVEMENT_EVENT_MAX });
    }
}

void ShowAchievementProgress(AchievementId achievementId, AchievementEvent triggeringEventId, bool fromEditor) {
    const Achievement* achievement = StaticData::GetAchievement(achievementId);
    if (!achievement) {
        return;
    }

    uint32_t current = 0;
    uint32_t max = 0;
    GetProgress(achievementId, current, max);

    if (max <= 1 || achievement->secret) {
        return;
    }

    if (fromEditor) {
        const Event* triggeringEvent = StaticData::GetEvent(triggeringEventId);
        const char* eventName = triggeringEvent ? triggeringEvent->name : "Unknown Event";
        Notification::EmitAchievementProgressWithEvent(achievement->iconPath ? achievement->iconPath : "", eventName,
                                                       achievement->name, current, max);
    } else {
        std::lock_guard<std::mutex> lock(achievementQueueMutex);
        achievementQueue.push_back(
            { SHOW_PROGRESS, AchievementEvent::ACHIEVEMENT_EVENT_MAX, achievementId, current, max, triggeringEventId });
    }
}
} // namespace

void Init() {
    StaticData::Init();
    Integration::Init();
    FileStorage::InitializeAchievementsFile();

    // Load achievement data if achievements are enabled
    if (CVarGetInteger("gEnhancements.Achievements.Enabled", 0)) {
        std::lock_guard<std::mutex> lock(achievementDataMutex);
        FileStorage::LoadAchievementsData(achievementData);
        RegisterAchievementTracker();
    }
}

void EnableAchievements() {
    CVarSetInteger("gEnhancements.Achievements.Enabled", 1);
    CVarSave();
    {
        std::lock_guard<std::mutex> lock(achievementDataMutex);
        FileStorage::LoadAchievementsData(achievementData);
    }
    RegisterAchievementTracker();
}

bool IsUnlocked(AchievementId achievementId) {
    if (!IS_ACHIEVEMENTS || !IsValidAchievementId(achievementId)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(achievementDataMutex);
    EnsureVectorsSized(achievementData);
    const int idIndex = static_cast<int>(achievementId);
    return achievementData.unlocked[idIndex];
}

bool IsEventTriggered(AchievementEvent achievementEventId) {
    if (!IS_ACHIEVEMENTS || !IsValidEventId(achievementEventId)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(achievementDataMutex);
    EnsureVectorsSized(achievementData);
    const int eventIndex = static_cast<int>(achievementEventId);
    return achievementData.events[eventIndex];
}

void GetProgress(AchievementId achievementId, uint32_t& current, uint32_t& max) {
    current = 0;
    max = 1;

    const Achievement* achievement = StaticData::GetAchievement(achievementId);
    if (!achievement) {
        return;
    }

    // Calculate max as sum of all required counts
    max = 0;
    for (const AchievementEvent requiredEvent : achievement->requiredEvents) {
        uint32_t requiredCount = 1;
        auto it = achievement->eventCounts.find(requiredEvent);
        if (it != achievement->eventCounts.end()) {
            requiredCount = it->second;
        }
        max += requiredCount;
    }

    if (max == 0) {
        max = 1;
    }

    // Calculate current as sum of actual progress (capped at required count per event)
    {
        std::lock_guard<std::mutex> lock(achievementDataMutex);
        EnsureVectorsSized(achievementData);
        for (const AchievementEvent requiredEvent : achievement->requiredEvents) {
            uint32_t requiredCount = 1;
            auto it = achievement->eventCounts.find(requiredEvent);
            if (it != achievement->eventCounts.end()) {
                requiredCount = it->second;
            }

            uint32_t eventCurrent = GetEventCounterUnlocked(requiredEvent, achievementData);
            // Cap at required count to avoid over-counting
            current += (eventCurrent < requiredCount) ? eventCurrent : requiredCount;
        }
    }
}

void EnsureDataLoaded() {
    std::lock_guard<std::mutex> lock(achievementDataMutex);
    // Check if data is already loaded by checking if vectors are sized
    if (achievementData.unlocked.size() < static_cast<size_t>(AchievementId::ACHIEVEMENT_ID_MAX) ||
        achievementData.events.size() < static_cast<size_t>(AchievementEvent::ACHIEVEMENT_EVENT_MAX)) {
        // Data not loaded, load it now
        FileStorage::LoadAchievementsData(achievementData);
    }
}

bool IsUnlockedReadOnly(AchievementId achievementId) {
    if (!IsValidAchievementId(achievementId)) {
        return false;
    }

    EnsureDataLoaded();

    std::lock_guard<std::mutex> lock(achievementDataMutex);
    EnsureVectorsSized(achievementData);
    const int idIndex = static_cast<int>(achievementId);
    return achievementData.unlocked[idIndex];
}

bool IsEventTriggeredReadOnly(AchievementEvent achievementEventId) {
    if (!IsValidEventId(achievementEventId)) {
        return false;
    }

    EnsureDataLoaded();

    std::lock_guard<std::mutex> lock(achievementDataMutex);
    EnsureVectorsSized(achievementData);
    const int eventIndex = static_cast<int>(achievementEventId);
    return achievementData.events[eventIndex];
}

void GetProgressReadOnly(AchievementId achievementId, uint32_t& current, uint32_t& max) {
    current = 0;
    max = 1;

    const Achievement* achievement = StaticData::GetAchievement(achievementId);
    if (!achievement) {
        return;
    }

    EnsureDataLoaded();

    // Calculate max as sum of all required counts
    max = 0;
    for (const AchievementEvent requiredEvent : achievement->requiredEvents) {
        uint32_t requiredCount = 1;
        auto it = achievement->eventCounts.find(requiredEvent);
        if (it != achievement->eventCounts.end()) {
            requiredCount = it->second;
        }
        max += requiredCount;
    }

    if (max == 0) {
        max = 1;
    }

    // Calculate current as sum of actual progress (capped at required count per event)
    {
        std::lock_guard<std::mutex> lock(achievementDataMutex);
        EnsureVectorsSized(achievementData);
        for (const AchievementEvent requiredEvent : achievement->requiredEvents) {
            uint32_t requiredCount = 1;
            auto it = achievement->eventCounts.find(requiredEvent);
            if (it != achievement->eventCounts.end()) {
                requiredCount = it->second;
            }

            uint32_t eventCurrent = GetEventCounterUnlocked(requiredEvent, achievementData);
            // Cap at required count to avoid over-counting
            current += (eventCurrent < requiredCount) ? eventCurrent : requiredCount;
        }
    }
}

uint32_t GetEventCounterReadOnly(AchievementEvent achievementEventId) {
    if (!IsValidEventId(achievementEventId)) {
        return 0;
    }

    EnsureDataLoaded();

    std::lock_guard<std::mutex> lock(achievementDataMutex);
    EnsureVectorsSized(achievementData);
    std::string key = GetEventCounterKey(achievementEventId);
    auto it = achievementData.counters.find(key);
    return (it != achievementData.counters.end()) ? it->second : 0;
}

void QueueEvent(AchievementEvent achievementEventId) {
    if (!IS_ACHIEVEMENTS) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(achievementQueueMutex);
        for (const auto& queuedEvent : achievementQueue) {
            if (queuedEvent.type == QUEUE_ACHIEVEMENT && queuedEvent.achievementEventId == achievementEventId) {
                return;
            }
        }

        achievementQueue.push_back({ QUEUE_ACHIEVEMENT, achievementEventId, AchievementId::ACHIEVEMENT_ID_MAX, 0, 0,
                                     AchievementEvent::ACHIEVEMENT_EVENT_MAX });
    }
}

void ProcessQueuedEvents() {
    if (!IS_ACHIEVEMENTS)
        return;

    Player* player = GET_PLAYER(gPlayState);
    if (!player)
        return;

    AchievementQueueEvent event;
    bool hasEvent = false;

    {
        std::lock_guard<std::mutex> lock(achievementQueueMutex);
        if (processing || achievementQueue.empty())
            return;

        AchievementQueueEvent& nextEvent = achievementQueue.front();
        if ((nextEvent.type == SHOW_UNLOCK || nextEvent.type == SHOW_PROGRESS) && Notification::IsNotificationActive())
            return;

        processing = true;
        event = achievementQueue.front();
        achievementQueue.erase(achievementQueue.begin());
        hasEvent = true;
    }

    if (!hasEvent)
        return;

    switch (event.type) {
        case QUEUE_ACHIEVEMENT:
            TriggerEvent(event.achievementEventId);
            break;

        case SHOW_UNLOCK: {
            const Achievement* achievement = StaticData::GetAchievement(event.achievementId);
            if (achievement) {
                Notification::EmitAchievement(achievement->iconPath ? achievement->iconPath : "",
                                              std::string(achievement->name), achievement->harbourMastery);
            }
            break;
        }

        case SHOW_PROGRESS: {
            const Achievement* achievement = StaticData::GetAchievement(event.achievementId);
            if (achievement) {
                const auto* triggeringEvent = StaticData::GetEvent(event.triggeringEventId);
                const char* eventName = triggeringEvent ? triggeringEvent->name : "Unknown Event";
                Notification::EmitAchievementProgressWithEvent(achievement->iconPath ? achievement->iconPath : "",
                                                               eventName, achievement->name, event.currentProgress,
                                                               event.maxProgress);
            }
            break;
        }

        default:
            SPDLOG_WARN("Unknown achievement queue event {}", static_cast<int>(event.type));
            break;
    }

    {
        std::lock_guard<std::mutex> lock(achievementQueueMutex);
        processing = false;
    }
}

void TriggerEvent(AchievementEvent achievementEventId, bool fromEditor) {
    if (!IS_ACHIEVEMENTS) {
        return;
    }

    if (!IsValidEventId(achievementEventId)) {
        return;
    }

    // Always increment counter (handles both single and multi-count events)
    IncrementEventCounter(achievementEventId);

    const Event* event = StaticData::GetEvent(achievementEventId);
    if (!event) {
        return;
    }

    for (const AchievementId achievementId : event->dependentAchievements) {
        if (IsUnlocked(achievementId)) {
            continue;
        }

        const Achievement* achievement = StaticData::GetAchievement(achievementId);
        if (!achievement) {
            continue;
        }

        if (IsAchievementComplete(achievement)) {
            UnlockAchievement(achievementId, fromEditor);
        } else {
            // Show progress for incomplete achievements
            // IsAchievementComplete check above prevents showing progress when it would unlock
            // ShowAchievementProgress filters out single-event achievements (max <= 1) to avoid spam
            ShowAchievementProgress(achievementId, achievementEventId, fromEditor);
        }
    }
}

void Lock(AchievementId achievementId) {
    if (!IS_ACHIEVEMENTS || !IsValidAchievementId(achievementId)) {
        return;
    }

    FileStorage::AchievementData dataCopy;
    {
        std::lock_guard<std::mutex> lock(achievementDataMutex);
        const int idIndex = static_cast<int>(achievementId);
        EnsureVectorsSized(achievementData);
        achievementData.unlocked[idIndex] = false;
        dataCopy = achievementData;
    }
    FileStorage::SaveAchievementsData(dataCopy);
}

void ResetEvent(AchievementEvent achievementEventId) {
    if (!IS_ACHIEVEMENTS) {
        return;
    }

    if (IsValidEventId(achievementEventId)) {
        // Reset counter (also handles boolean flag reset for backward compatibility)
        ResetEventCounter(achievementEventId);
    }

    const Event* event = StaticData::GetEvent(achievementEventId);
    if (!event) {
        return;
    }

    for (const AchievementId achievementId : event->dependentAchievements) {
        Lock(achievementId);
    }
}

void SetEventCounter(AchievementEvent achievementEventId, uint32_t count, bool fromEditor) {
    if (!IS_ACHIEVEMENTS) {
        return;
    }

    if (!IsValidEventId(achievementEventId)) {
        return;
    }

    // Set counter (also handles boolean flag update for backward compatibility)
    SetEventCounterValue(achievementEventId, count);

    const Event* event = StaticData::GetEvent(achievementEventId);
    if (!event) {
        return;
    }

    if (count > 0) {
        // Check for achievement unlocks (like TriggerEvent)
        for (const AchievementId achievementId : event->dependentAchievements) {
            if (IsUnlocked(achievementId)) {
                continue;
            }

            const Achievement* achievement = StaticData::GetAchievement(achievementId);
            if (!achievement) {
                continue;
            }

            if (IsAchievementComplete(achievement)) {
                UnlockAchievement(achievementId, fromEditor);
            } else {
                // Show progress for incomplete achievements
                // IsAchievementComplete check above prevents showing progress when it would unlock
                // ShowAchievementProgress filters out single-event achievements (max <= 1) to avoid spam
                ShowAchievementProgress(achievementId, achievementEventId, fromEditor);
            }
        }
    } else {
        // Lock dependent achievements (like ResetEvent)
        for (const AchievementId achievementId : event->dependentAchievements) {
            Lock(achievementId);
        }
    }
}

void RegisterAchievementTracker() {
    COND_ID_HOOK(OnActorUpdate, ACTOR_PLAYER, IS_ACHIEVEMENTS,
                 [](Actor* actor) { Achievements::ProcessQueuedEvents(); });

    COND_HOOK(OnFlagSet, IS_ACHIEVEMENTS,
              [](FlagType flagType, u32 flag) { Achievements::Integration::OnFlagSet(flagType, flag); });

    COND_HOOK(OnSceneFlagSet, IS_ACHIEVEMENTS, [](s16 sceneId, FlagType flagType, u32 flag) {
        Achievements::Integration::OnSceneFlagSet(sceneId, flagType, flag);
    });

    COND_HOOK(OnBossDefeated, IS_ACHIEVEMENTS, [](s16 actorId) { Achievements::Integration::OnBossDefeated(actorId); });

    for (const auto& [vbFlag, conditions] : Achievements::Integration::vanillaBehaviorMap) {
        COND_VB_SHOULD(vbFlag, IS_ACHIEVEMENTS, { Achievements::Integration::OnVanillaBehavior(_, should, args); });
    }
}

static RegisterShipInitFunc initFunc(RegisterAchievementTracker, { "gEnhancements.Achievements.Enabled" });

} // namespace Achievements
