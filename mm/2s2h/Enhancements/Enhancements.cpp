#include "Enhancements.h"

void InitEnhancements() {
    // Camera
    RegisterCameraInterpolationFixes();
    RegisterCameraFreeLook();
    RegisterDebugCam();

    // Cheats
    RegisterInfiniteCheats();
    RegisterLongerFlowerGlide();
    RegisterMoonJumpOnL();
    RegisterUnbreakableRazorSword();
    RegisterUnrestrictedItems();
    RegisterTimeStopInTemples();
    RegisterElegyAnywhere();
    RegisterHookshotAnywhere();

    // Clock
    RegisterTextBasedClock();
    Register3DSClock();

    // Cycle & Saving
    RegisterEndOfCycleSaveHooks();
    RegisterSavingEnhancements();
    RegisterAutosave();
    RegisterKeepExpressMail();

    // Dialogue
    RegisterFastBankSelection();

    // Equipment
    RegisterSkipMagicArrowEquip();
    RegisterGreatFairySwordOnB();
    RegisterInstantRecall();

    // Fixes
    RegisterFierceDeityZTargetMovement();
    RegisterTwoHandedSwordSpinAttack();
    RegisterCompletedHeartContainerAudio();

    // Graphics
    RegisterDisableBlackBars();
    Register3DItemDrops();

    // Masks
    RegisterFastTransformation();
    RegisterFierceDeityAnywhere();
    RegisterBlastMaskKeg();
    RegisterNoBlastMaskCooldown();
    RegisterPersistentMasks();
    RegisterEasyMaskEquip();

    // Minigames
    RegisterAlwaysWinDoggyRace();
    RegisterCremiaHugs();
    RegisterSwordsmanSchool();
    RegisterCuccoShackCuccoCount();

    // Player
    RegisterClimbSpeed();
    RegisterFastFlowerLaunch();
    RegisterInstantPutaway();
    RegisterFierceDeityPutaway();
    RegisterManualJump();

    // Songs
    RegisterEnableSunsSong();
    RegisterFasterSongPlayback();
    RegisterPauseOwlWarp();
    RegisterZoraEggCount();
    RegisterSkipScarecrowSong();

    // Restorations
    RegisterPowerCrouchStab();
    RegisterSideRoll();
    RegisterTatlISG();
    RegisterVariableFlipHop();
    RegisterWoodfallMountainAppearance();
    RegisterShowDekuGuardSearchBalls();

    // Cutscenes
    RegisterCutscenes();

    // Modes
    RegisterPlayAsKafei();
    RegisterTimeMovesWhenYouMove();

    // Difficulty Options
    RegisterDisableTakkuriSteal();
}
