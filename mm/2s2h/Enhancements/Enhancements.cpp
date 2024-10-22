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
    RegisterHookshotAnywhere();
    RegisterElegyAnywhere();

    // Clock
    RegisterTextBasedClock();
    Register3DSClock();

    // Cycle & Saving
    RegisterEndOfCycleSaveHooks();
    RegisterSavingEnhancements();
    RegisterAutosave();
    RegisterKeepExpressMail();
    RegisterSkipScarecrowSong();

    // Dialogue
    RegisterFastBankSelection();

    // Equipment
    RegisterSkipMagicArrowEquip();
    RegisterTwoHandedSwordSpinAttack();
    RegisterGreatFairySwordOnB();
    RegisterInstantRecall();

    // Fixes
    RegisterFierceDeityZTargetMovement();

    // Graphics
    RegisterDisableBlackBars();
    RegisterHyruleWarriorsStyledLink();
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
    RegisterSwordsmanSchool();
    RegisterCremiaHugs();

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

    // Cutscenes
    RegisterCutscenes();

    // Modes
    RegisterInvisibleEnemies();
    RegisterPlayAsKafei();
    RegisterTimeMovesWhenYouMove();

    // Difficulty Options
    RegisterDisableTakkuriSteal();
}
