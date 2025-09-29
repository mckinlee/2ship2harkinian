#include "ClockShuffle.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "Rando/Rando.h"

#include <algorithm>
#include <array>
#include <iterator>

extern "C" {
#include "z64game.h"
}

namespace Rando {
namespace {

constexpr int kInvalidHalfDay = -1;

} // namespace

// ============================================================================
// CLOCK ITEM MANAGEMENT
// ============================================================================

namespace ClockItems {

enum ClockHalfIndex : int {
    HALF_DAY1_DAY = 0,
    HALF_DAY1_NIGHT = 1,
    HALF_DAY2_DAY = 2,
    HALF_DAY2_NIGHT = 3,
    HALF_DAY3_DAY = 4,
    HALF_DAY3_NIGHT = 5,
    TERMINAL_STATE = 6,
    HALF_COUNT = 6,
};

constexpr std::array<RandoItemId, HALF_COUNT> kHalfDayItems = {
    RI_CLOCK_DAY_1,
    RI_CLOCK_NIGHT_1,
    RI_CLOCK_DAY_2,
    RI_CLOCK_NIGHT_2,
    RI_CLOCK_DAY_3,
    RI_CLOCK_NIGHT_3,
};

constexpr std::array<RandoInf, HALF_COUNT> kHalfDayFlags = {
    static_cast<RandoInf>(RANDO_INF_OBTAINED_CLOCK_DAY_1 + 0),
    static_cast<RandoInf>(RANDO_INF_OBTAINED_CLOCK_DAY_1 + 1),
    static_cast<RandoInf>(RANDO_INF_OBTAINED_CLOCK_DAY_1 + 2),
    static_cast<RandoInf>(RANDO_INF_OBTAINED_CLOCK_DAY_1 + 3),
    static_cast<RandoInf>(RANDO_INF_OBTAINED_CLOCK_DAY_1 + 4),
    static_cast<RandoInf>(RANDO_INF_OBTAINED_CLOCK_DAY_1 + 5),
};

constexpr bool IsValidHalfDay(int halfDayIndex) {
    return halfDayIndex >= 0 && halfDayIndex < HALF_COUNT;
}

bool OwnsHalfDay(int halfDayIndex) {
    return IsValidHalfDay(halfDayIndex) && Flags_GetRandoInf(kHalfDayFlags[halfDayIndex]);
}

int GetHalfDayIndexFromClockItem(RandoItemId clockItemId) {
    const auto it = std::find(kHalfDayItems.begin(), kHalfDayItems.end(), clockItemId);
    if (it == kHalfDayItems.end()) {
        return kInvalidHalfDay;
    }

    return static_cast<int>(std::distance(kHalfDayItems.begin(), it));
}

RandoItemId GetClockItemFromHalfDayIndex(int halfDayIndex) {
    if (!IsValidHalfDay(halfDayIndex)) {
        return RI_UNKNOWN;
    }

    return kHalfDayItems[halfDayIndex];
}

u8 GetAllOwnedHalfDaysMask() {
    u8 ownedMask = 0;
    for (int halfDay = 0; halfDay < HALF_COUNT; ++halfDay) {
        if (OwnsHalfDay(halfDay)) {
            ownedMask |= (1 << halfDay);
        }
    }

    return ownedMask;
}

int FindEarliestOwnedHalfDay(bool searchFromEnd) {
    if (searchFromEnd) {
        for (int halfDay = HALF_COUNT - 1; halfDay >= 0; --halfDay) {
            if (OwnsHalfDay(halfDay)) {
                return halfDay;
            }
        }
    } else {
        for (int halfDay = 0; halfDay < HALF_COUNT; ++halfDay) {
            if (OwnsHalfDay(halfDay)) {
                return halfDay;
            }
        }
    }

    return kInvalidHalfDay;
}

int FindNextOwnedHalfDayAfter(int startHalfDay, u8 ownedMask) {
    if (startHalfDay < kInvalidHalfDay || startHalfDay >= HALF_COUNT) {
        return TERMINAL_STATE;
    }

    for (int halfDay = startHalfDay + 1; halfDay < HALF_COUNT; ++halfDay) {
        if (ownedMask & (1 << halfDay)) {
            return halfDay;
        }
    }

    return TERMINAL_STATE;
}

} // namespace ClockItems

namespace {

struct HalfDayTimeConfig {
    u8 dayNumber;
    u16 startTime;
    u16 endTime;
};

constexpr u16 DAWN_TIME = CLOCK_TIME(6, 0);
constexpr u16 DUSK_TIME = CLOCK_TIME(18, 0);
constexpr u16 DAWN_END_TIME = CLOCK_TIME(5, 59);
constexpr u16 DUSK_END_TIME = CLOCK_TIME(17, 59);
constexpr u16 TERMINAL_STATE_TIME = CLOCK_TIME(0, 0);
constexpr u16 DAY_0_0559_TIME = CLOCK_TIME(6, 0) - 1; // Vanilla uses 16383 for the reset.

const HalfDayTimeConfig* GetHalfDayTimeConfig(int halfDayIndex) {
    static constexpr std::array<HalfDayTimeConfig, ClockItems::HALF_COUNT> kHalfDayConfigs = {
        HalfDayTimeConfig{ 1, DAWN_TIME, DUSK_END_TIME },
        HalfDayTimeConfig{ 1, DUSK_TIME, DAWN_END_TIME },
        HalfDayTimeConfig{ 2, DAWN_TIME, DUSK_END_TIME },
        HalfDayTimeConfig{ 2, DUSK_TIME, DAWN_END_TIME },
        HalfDayTimeConfig{ 3, DAWN_TIME, DUSK_END_TIME },
        HalfDayTimeConfig{ 3, DUSK_TIME, DAWN_END_TIME },
    };

    if (!ClockItems::IsValidHalfDay(halfDayIndex)) {
        return nullptr;
    }

    return &kHalfDayConfigs[halfDayIndex];
}

bool IsCurrentlyNightTime(u16 gameTime) {
    return (gameTime >= DUSK_TIME) || (gameTime < DAWN_TIME);
}

struct ClockShuffleState {
    int lastKnownHalfDay = kInvalidHalfDay;
    bool isRedirecting = false;
    HOOK_ID playDestroyHook = 0;
    u8 preservedHopCounter = 0;
};

ClockShuffleState sState{};

void ApplyGameTime(u8 day, u16 time) {
    gSaveContext.save.day = day;
    gSaveContext.save.time = time;
    gSaveContext.save.isNight = IsCurrentlyNightTime(time);
    gSaveContext.save.eventDayCount = day;
}

void UpdateSceneSequenceForTime(u16 time) {
    gSaveContext.seqId = NA_BGM_DISABLED;
    gSceneSeqState = IsCurrentlyNightTime(time) ? SCENESEQ_DEFAULT : SCENESEQ_MORNING;
}

void ForceSceneReload() {
    Player* player = GET_PLAYER(gPlayState);

    gPlayState->nextEntrance = gSaveContext.save.entrance;
    gPlayState->transitionTrigger = TRANS_TRIGGER_START;
    gPlayState->transitionType = TRANS_TYPE_FADE_BLACK_FAST;

    Play_SetRespawnData(&gPlayState->state, RESPAWN_MODE_RETURN, gSaveContext.save.entrance,
                        gPlayState->roomCtx.curRoom.num, PLAYER_PARAMS(0xFF, PLAYER_INITMODE_B),
                        &player->actor.world.pos, player->actor.world.rot.y);

    gSaveContext.nextTransitionType = TRANS_TYPE_FADE_BLACK;
    gSaveContext.respawnFlag = 2;
}

void PreserveDekuHopCounter() {
    Player* player = GET_PLAYER(gPlayState);
    if (player != nullptr && player->transformation == PLAYER_FORM_DEKU) {
        sState.preservedHopCounter = player->remainingHopsCounter;
    } else {
        sState.preservedHopCounter = 0;
    }
}

void RestorePreservedDekuHopCounter(Actor* actor) {
    if (sState.preservedHopCounter == 0) {
        return;
    }

    Player* player = reinterpret_cast<Player*>(actor);
    if (player->transformation == PLAYER_FORM_DEKU) {
        player->remainingHopsCounter = sState.preservedHopCounter;
    }

    sState.preservedHopCounter = 0;
}

} // namespace

namespace ClockShuffle {

// ============================================================================
// TIME AND STATE HELPERS
// ============================================================================

int GetCurrentHalfDayIndex() {
    const u16 currentTime = gSaveContext.save.time;
    const s32 currentDay = gSaveContext.save.day;

    if (currentDay >= 4) {
        return ClockItems::TERMINAL_STATE;
    }

    if (currentDay == 0) {
        return ClockItems::TERMINAL_STATE;
    }

    if (currentDay == 3 && currentTime >= TERMINAL_STATE_TIME && currentTime < DAWN_TIME) {
        return ClockItems::TERMINAL_STATE;
    }

    const bool isNight = IsCurrentlyNightTime(currentTime);
    return (currentDay - 1) * 2 + (isNight ? 1 : 0);
}

void SetTimeToHalfDayStart(int halfDayIndex) {
    if (halfDayIndex == ClockItems::TERMINAL_STATE) {
        return;
    }

    const HalfDayTimeConfig* config = GetHalfDayTimeConfig(halfDayIndex);
    if (config == nullptr) {
        return;
    }

    ApplyGameTime(config->dayNumber, config->startTime);
    UpdateSceneSequenceForTime(config->startTime);
}

// ============================================================================
// TRANSITION MANAGEMENT
// ============================================================================

void CompleteRedirect(int targetHalfDay) {
    if (targetHalfDay == ClockItems::TERMINAL_STATE) {
        ApplyGameTime(3, TERMINAL_STATE_TIME);
        UpdateSceneSequenceForTime(TERMINAL_STATE_TIME);
    } else {
        SetTimeToHalfDayStart(targetHalfDay);
    }

    sState.lastKnownHalfDay = targetHalfDay;
    sState.isRedirecting = false;
}

void ProcessHalfDayTransition(Actor* /*timeActor*/, int fromHalfDay, int toHalfDay) {
    const bool playerOwnsTarget =
        (toHalfDay == ClockItems::TERMINAL_STATE || !ClockItems::IsValidHalfDay(toHalfDay))
            ? true
            : ClockItems::OwnsHalfDay(toHalfDay);

    if (playerOwnsTarget) {
        return;
    }

    PreserveDekuHopCounter();

    const u8 ownedHalfDaysMask = ClockItems::GetAllOwnedHalfDaysMask();
    const int nextOwnedHalfDay = ClockItems::FindNextOwnedHalfDayAfter(fromHalfDay, ownedHalfDaysMask);

    sState.isRedirecting = true;
    sState.playDestroyHook = GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([nextOwnedHalfDay]() {
        const HOOK_ID hookId = sState.playDestroyHook;
        CompleteRedirect(nextOwnedHalfDay);
        GameInteractor::Instance->UnregisterGameHook<GameInteractor::OnPlayDestroy>(hookId);
        sState.playDestroyHook = 0;
    });

    ForceSceneReload();
}

void OnTimeTransitionDetected(Actor* timeActor, bool* should) {
    if (gSaveContext.save.day >= 4) {
        return;
    }

    const int currentHalfDay = GetCurrentHalfDayIndex();

    if (sState.lastKnownHalfDay != kInvalidHalfDay && currentHalfDay != sState.lastKnownHalfDay) {
        if (sState.isRedirecting) {
            return;
        }

        const bool isTargetTerminalState = (currentHalfDay == ClockItems::TERMINAL_STATE);
        const bool isFromTerminalState = (sState.lastKnownHalfDay == ClockItems::TERMINAL_STATE);
        const bool playerOwnsTarget =
            isTargetTerminalState ? true : ClockItems::OwnsHalfDay(currentHalfDay);

        if (isFromTerminalState) {
            sState.lastKnownHalfDay = currentHalfDay;
            return;
        }

        if (!playerOwnsTarget) {
            *should = false;
            ProcessHalfDayTransition(timeActor, sState.lastKnownHalfDay, currentHalfDay);
            return;
        }
    }

    sState.lastKnownHalfDay = currentHalfDay;
}

// ============================================================================
// PUBLIC API
// ============================================================================

void OnFileLoad() {
    COND_ID_HOOK(ShouldActorUpdate, ACTOR_EN_TEST4, RANDO_SAVE_OPTIONS[RO_CLOCK_SHUFFLE],
                 [](Actor* actor, bool* should) { OnTimeTransitionDetected(actor, should); });

    if (gPlayState == nullptr && !gSaveContext.save.isOwlSave) {
        const int earliestOwnedHalfDay = ClockItems::FindEarliestOwnedHalfDay(false);
        if (earliestOwnedHalfDay != kInvalidHalfDay) {
            SetTimeToHalfDayStart(earliestOwnedHalfDay);
        }
    }

    COND_HOOK(OnPlayDestroy, IS_RANDO && RANDO_SAVE_OPTIONS[RO_CLOCK_SHUFFLE], []() {
        if (gSaveContext.save.day == 0 && gSaveContext.save.time == DAY_0_0559_TIME) {
            if (!ClockItems::OwnsHalfDay(ClockItems::HALF_DAY1_DAY)) {
                const int earliestOwnedHalfDay = ClockItems::FindEarliestOwnedHalfDay(false);
                if (earliestOwnedHalfDay != kInvalidHalfDay) {
                    SetTimeToHalfDayStart(earliestOwnedHalfDay);
                }
            }
        }
    });

    COND_ID_HOOK(OnActorInit, ACTOR_PLAYER, IS_RANDO && RANDO_SAVE_OPTIONS[RO_CLOCK_SHUFFLE],
                 [](Actor* actor) { RestorePreservedDekuHopCounter(actor); });
}

} // namespace ClockShuffle

} // namespace Rando
