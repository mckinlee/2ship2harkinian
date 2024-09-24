#ifndef ENHANCEMENTS_H
#define ENHANCEMENTS_H

#include "Camera/CameraInterpolationFixes.h"
#include "Camera/DebugCam.h"
#include "Camera/FreeLook.h"
#include "Cheats/MoonJump.h"
#include "Cheats/Infinite.h"
#include "Dialogue/Dialogue.h"
#include "DifficultyOptions/DisableTakkuriSteal.h"
#include "Graphics/TextBasedClock.h"
#include "Graphics/3DSClock.h"
#include "Cheats/Cheats.h"
#include "Cheats/UnbreakableRazorSword.h"
#include "Cheats/UnrestrictedItems.h"
#include "Cheats/TimeStop.h"
#include "Cycle/EndOfCycle.h"
#include "Cycle/BothLetterToMamaRewards.h"
#include "Equipment/SkipMagicArrowEquip.h"
#include "Fixes/Fixes.h"
#include "Masks/BlastMaskKeg.h"
#include "Masks/FierceDeityAnywhere.h"
#include "Masks/NoBlastMaskCooldown.h"
#include "Masks/FastTransformation.h"
#include "Masks/PersistentMasks.h"
#include "Masks/EasyMaskEquip.h"
#include "Minigames/AlwaysWinDoggyRace.h"
#include "Cutscenes/Cutscenes.h"
#include "Restorations/FlipHopVariable.h"
#include "Restorations/PowerCrouchStab.h"
#include "Restorations/Restorations.h"
#include "Restorations/SideRoll.h"
#include "Restorations/TatlISG.h"
#include "Graphics/3DItemDrops.h"
#include "Graphics/PlayAsKafei.h"
#include "Equipment/InstantRecall.h"
#include "Player/Player.h"
#include "Songs/EnableSunsSong.h"
#include "Songs/PauseOwlWarp.h"
#include "Songs/ZoraEggCount.h"
#include "Saving/SavingEnhancements.h"
#include "Graphics/DisableBlackBars.h"
#include "Modes/TimeMovesWhenYouMove.h"

enum AlwaysWinDoggyRaceOptions {
    ALWAYS_WIN_DOGGY_RACE_OFF,
    ALWAYS_WIN_DOGGY_RACE_MASKOFTRUTH,
    ALWAYS_WIN_DOGGY_RACE_ALWAYS,
};

enum TimeStopOptions {
    TIME_STOP_OFF,
    TIME_STOP_TEMPLES,
    TIME_STOP_TEMPLES_DUNGEONS,
};

enum ClockTypeOptions {
    CLOCK_TYPE_ORIGINAL,
    CLOCK_TYPE_3DS,
    CLOCK_TYPE_TEXT_BASED,
};

#ifdef __cplusplus
extern "C" {
#endif

void InitEnhancements();

#ifdef __cplusplus
}
#endif

#endif // ENHANCEMENTS_H
