#include "HookshotAnywhere.h"
#include <libultraship/bridge.h>
#include "2s2h/GameInteractor/GameInteractor.h"

void RegisterHookshotAnywhere() {
    REGISTER_VB_SHOULD(GI_VB_ENABLE_HOOKSHOT_ANYWHERE, {
        if (CVarGetInteger("gCheats.HookshotAnywhere", 0)) {
            *should = true;
        }
    });
}