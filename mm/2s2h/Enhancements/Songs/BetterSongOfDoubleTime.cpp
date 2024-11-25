#include <libultraship/bridge.h>
#include "2s2h/GameInteractor/GameInteractor.h"
#include "spdlog/spdlog.h"

extern "C" {
#include "macros.h"
#include "assets/interface/week_static/week_static.h"
extern PlayState* gPlayState;
extern SaveContext gSaveContext;
}

static bool activelyChangingTime = false;
static u32 originalTime = CLOCK_TIME(0, 0);
static u32 originalDay = 0;

extern void UpdateGameTime(u16 gameTime);

const u32 INTERVAL = (CLOCK_TIME_MINUTE * 30);

static const char* sDoWeekTableCopy[] = {
    gClockDay1stTex,
    gClockDay2ndTex,
    gClockDayFinalTex,
};

static HOOK_ID onPlayerUpdateHookId = 0;

void OnPlayerUpdate(Actor* actor) {
    if (!activelyChangingTime) {
        GameInteractor::Instance->UnregisterGameHookForID<GameInteractor::OnActorUpdate>(onPlayerUpdateHookId);
        return;
    }

    gPlayState->interfaceCtx.bAlpha = 255;

    Input* input = &gPlayState->state.input[0];

    // Pressing B should cancel the song
    if (CHECK_BTN_ALL(input->press.button, BTN_B)) {
        Audio_PlaySfx_MessageCancel();
        gPlayState->msgCtx.ocarinaMode = OCARINA_MODE_END;
        activelyChangingTime = false;

        gSaveContext.save.day = originalDay;
        UpdateGameTime(originalTime);
        Interface_NewDay(gPlayState, CURRENT_DAY);
        // This may have happened in the UpdateGameTime function, if there was a day/night difference, but
        // we need to ensure it happens regardless because the day may have changed even if the time is the same
        gPlayState->numSetupActors = ABS(gPlayState->numSetupActors);
        // Load environment values for new day
        func_800FEAF4(&gPlayState->envCtx);
    }

    // Pressing A should confirm the song
    if (CHECK_BTN_ALL(input->press.button, BTN_A)) {
        Audio_PlaySfx_MessageDecide();
        gPlayState->msgCtx.ocarinaMode = OCARINA_MODE_END;
        activelyChangingTime = false;

        gSaveContext.save.eventDayCount = CURRENT_DAY;
    }

    // Analog stick should change the time
    if (input->cur.stick_x > 0) { // Advance time
        u16 newTime = CLAMP(gSaveContext.save.time + INTERVAL, 0,
                            (gSaveContext.save.day == 3 && gSaveContext.save.time < CLOCK_TIME(6, 0))
                                ? (CLOCK_TIME(6, 0) - CLOCK_TIME_HOUR)
                                : INFINITY);
        if (newTime > CLOCK_TIME(6, 0) && gSaveContext.save.time < CLOCK_TIME(6, 0)) {
            gSaveContext.save.day = CLAMP(gSaveContext.save.day + 1, originalDay, 3);
            Interface_NewDay(gPlayState, CURRENT_DAY);
            func_800FEAF4(&gPlayState->envCtx);
        }
        UpdateGameTime(newTime);
    } else if (input->cur.stick_x < 0) { // Reverse time
        u16 newTime = CLAMP(gSaveContext.save.time - INTERVAL,
                            (gSaveContext.save.day == originalDay &&
                             ((gSaveContext.save.time > CLOCK_TIME(6, 0) && originalTime > CLOCK_TIME(6, 0)) ||
                              (gSaveContext.save.time < CLOCK_TIME(6, 0) && originalTime < CLOCK_TIME(6, 0))))
                                ? originalTime
                                : 0,
                            INFINITY);
        if (newTime < CLOCK_TIME(6, 0) && gSaveContext.save.time > CLOCK_TIME(6, 0)) {
            gSaveContext.save.day = CLAMP(gSaveContext.save.day - 1, originalDay, 3);
            Interface_NewDay(gPlayState, CURRENT_DAY);
            func_800FEAF4(&gPlayState->envCtx);
        }
        UpdateGameTime(newTime);
    }
}

void RegisterBetterSongOfDoubleTime() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](s8 sceneId, s8 spawnNum) {
        // In case we didn't properly reset this variable
        activelyChangingTime = false;
        originalTime = CLOCK_TIME(0, 0);
        originalDay = 0;
    });

    REGISTER_VB_SHOULD(VB_DISPLAY_SONG_OF_DOUBLE_TIME_PROMPT, {
        if (CVarGetInteger("gEnhancements.Songs.BetterSongOfDoubleTime", 0)) {
            *should = false;
            gPlayState->msgCtx.ocarinaMode = OCARINA_MODE_PROCESS_DOUBLE_TIME;
            activelyChangingTime = true;
            originalTime = gSaveContext.save.time;
            originalDay = gSaveContext.save.day;

            onPlayerUpdateHookId = GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnActorUpdate>(
                ACTOR_PLAYER, OnPlayerUpdate);
        }
    });
}
