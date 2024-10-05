#include <libultraship/bridge.h>
#include "2s2h/GameInteractor/GameInteractor.h"

extern "C" {
bool isPressingHeldItemButton(Player* player);
}

void RegisterTwoHandedSwordSpinAttack() {

    REGISTER_VB_SHOULD(GI_VB_HELD_ITEM_BUTTON_PRESS, {
        if (CVarGetInteger("gEnhancements.Equipment.TwoHandedSwordSpinAttack", 0)) {
            // Allow the Great Fairy Sword, a C button item, to be held for charged spin attacks
            Player* player = (Player*)(va_arg(args, Player*)); // Retrieve the player from va_list
            *should = isPressingHeldItemButton(player);
        }
    });
    REGISTER_VB_SHOULD(GI_VB_MAGIC_SPIN_ATTACK_FORM_CHECK, {
        if (CVarGetInteger("gEnhancements.Equipment.TwoHandedSwordSpinAttack", 0)) {
            // Allow both Fierce Deity and human forms to use charged spin attacks
            uint8_t form = va_arg(args, int);
            *should = (form == PLAYER_FORM_FIERCE_DEITY || form == PLAYER_FORM_HUMAN);
        }
    });
}
