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

    // Player
    RegisterClimbSpeed();
    RegisterFastFlowerLaunch();
    RegisterInstantPutaway();
    RegisterFierceDeityPutaway();

    // Songs
    RegisterEnableSunsSong();
    RegisterPauseOwlWarp();
    RegisterZoraEggCount();

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
}
