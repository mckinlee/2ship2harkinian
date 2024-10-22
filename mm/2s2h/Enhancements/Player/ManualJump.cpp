#include <libultraship/bridge.h>
#include "2s2h/GameInteractor/GameInteractor.h"

void RegisterManualJump() {
    REGISTER_VB_SHOULD(VB_MANUAL_JUMP, {
        if (CVarGetInteger("gEnhancements.Player.ManualJump", 0)) {
            *should = false;
        }
    });
}