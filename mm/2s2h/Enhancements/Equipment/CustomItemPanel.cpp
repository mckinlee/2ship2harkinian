#include <libultraship/bridge/consolevariablebridge.h>
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include <string.h>
#include <math.h>
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "2s2h_assets.h"

extern "C" {
#include "z64save.h"
#include "variables.h"
#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"

#include "Enhancements/Items/Custom_Magic_Fire.h"
void Player_InitDefaultIA(PlayState* play, Player* thisx);
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define CVAR_CUSTOM_PANEL_ENABLED_NAME "gEnhancements.Equipment.CustomItemPanel"
#define CVAR_CUSTOM_PANEL_ENABLED CVarGetInteger(CVAR_CUSTOM_PANEL_ENABLED_NAME, 0)
#define MAGIC_COST_DINS_FIRE (MAGIC_NORMAL_METER / 10)

static bool sShowingCustomItems = false;
static u8 sCustomItems[48];
static s8 sCustomAmmo[24];

// Backup for swapping
static u8 sBackupItems[48];
static s8 sBackupAmmo[24];
static bool sIsSwapped = false;

// Button backups for outline pruning
static u8 sBackupButtonItems[3];
static bool sHasButtonBackup = false;
static u8 sBackupDpadItems[4];
static bool sHasDpadBackup = false;

// Animation state
static float sFlipAngle = 0.0f;
static float sTargetFlipAngle = 0.0f;

typedef enum {
    CUSTOM_ITEM_NONE,
    CUSTOM_ITEM_DINS_FIRE,
} CustomItemId;

#define ITEM_CUSTOM_MARKER_B 0xD0
#define ITEM_CUSTOM_MARKER_C_LEFT 0xD1
#define ITEM_CUSTOM_MARKER_C_DOWN 0xD2
#define ITEM_CUSTOM_MARKER_C_RIGHT 0xD3
#define ITEM_CUSTOM_MARKER_DINS_FIRE 0xD4

#define IS_CUSTOM_MARKER(id) ((id) >= ITEM_CUSTOM_MARKER_B && (id) <= ITEM_CUSTOM_MARKER_DINS_FIRE)
#define IS_HUD_MARKER(id) ((id) >= ITEM_CUSTOM_MARKER_B && (id) <= ITEM_CUSTOM_MARKER_C_RIGHT)

struct CustomItemEntry {
    CustomItemId customItemId;
    u8 slot;
    u8 markerId;
    std::function<bool()> canUse;
    std::function<void()> onUse;
    std::function<void*()> getIcon;
};

static std::vector<CustomItemEntry> sCustomItemRegistry;

void RegisterCustomItem(CustomItemId customItemId, u8 slot, u8 markerId, std::function<bool()> canUse,
                        std::function<void()> onUse, std::function<void*()> getIcon) {
    sCustomItemRegistry.push_back({ customItemId, slot, markerId, canUse, onUse, getIcon });
}

void InitCustomItems() {
    sCustomItemRegistry.clear();

    // Register Custom Din's Fire
    RegisterCustomItem(
        CUSTOM_ITEM_DINS_FIRE, SLOT_ARROW_FIRE, ITEM_CUSTOM_MARKER_DINS_FIRE,
        []() { return gSaveContext.save.saveInfo.playerData.magic >= MAGIC_COST_DINS_FIRE; },
        []() {
            Player* player = GET_PLAYER(gPlayState);
            Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_CUSTOM_SPELL_FIRE, player->actor.world.pos.x,
                        player->actor.world.pos.y, player->actor.world.pos.z, 0, player->actor.shape.rot.y, 0, 0);
            gSaveContext.save.saveInfo.playerData.magic -= MAGIC_COST_DINS_FIRE;
        },
        []() -> void* { return (void*)gDinsFireIcon; });
}

void RegisterCustomItemPanel() {
    InitCustomItems();

    // Initialize custom items
    for (int i = 0; i < 48; i++) {
        sCustomItems[i] = ITEM_NONE;
    }
    for (int i = 0; i < 24; i++) {
        sCustomAmmo[i] = 0;
    }

    // Populate from registry
    for (const auto& entry : sCustomItemRegistry) {
        if (entry.slot < 48) {
            sCustomItems[entry.slot] = entry.markerId;
        }
    }

    // Use GameState hooks for the inventory swap to ensure it covers the entire update and draw cycle.
    COND_HOOK(OnGameStateMainStart, CVAR_CUSTOM_PANEL_ENABLED, []() {
        // Detect custom item equips
        for (int i = 0; i < 4; i++) {
            u8 itemOnButton = gSaveContext.save.saveInfo.equips.buttonItems[0][i];

            // If it's an inventory marker, we need to convert it to a slot marker and track it
            if (itemOnButton >= ITEM_CUSTOM_MARKER_DINS_FIRE) {
                CustomItemId customItemId = CUSTOM_ITEM_NONE;
                for (const auto& entry : sCustomItemRegistry) {
                    if (entry.markerId == itemOnButton) {
                        customItemId = entry.customItemId;
                        break;
                    }
                }

                if (customItemId != CUSTOM_ITEM_NONE) {
                    gSaveContext.save.shipSaveInfo.customEquips[i] = customItemId;
                    gSaveContext.save.saveInfo.equips.buttonItems[0][i] = (ITEM_CUSTOM_MARKER_B + i);
                }
            } else if (itemOnButton < ITEM_CUSTOM_MARKER_B || itemOnButton > ITEM_CUSTOM_MARKER_C_RIGHT) {
                // Vanilla item equipped, clear custom tracking
                gSaveContext.save.shipSaveInfo.customEquips[i] = CUSTOM_ITEM_NONE;
            }
        }

        if (gPlayState != NULL && IS_PAUSED(&gPlayState->pauseCtx) && gPlayState->pauseCtx.pageIndex == PAUSE_ITEM &&
            sShowingCustomItems) {

            // Backup vanilla items
            memcpy(sBackupItems, gSaveContext.save.saveInfo.inventory.items, sizeof(sBackupItems));
            memcpy(sBackupAmmo, gSaveContext.save.saveInfo.inventory.ammo, sizeof(sBackupAmmo));

            // Swap to custom items
            memcpy(gSaveContext.save.saveInfo.inventory.items, sCustomItems, sizeof(sCustomItems));
            memcpy(gSaveContext.save.saveInfo.inventory.ammo, sCustomAmmo, sizeof(sCustomAmmo));
            sIsSwapped = true;
        }
    });

    COND_HOOK(OnGameStateUpdate, CVAR_CUSTOM_PANEL_ENABLED, []() {
        // Always attempt to restore if we swapped, regardless of CVar state (safety)
        if (sIsSwapped) {
            // Restore vanilla items
            memcpy(gSaveContext.save.saveInfo.inventory.items, sBackupItems, sizeof(sBackupItems));
            memcpy(gSaveContext.save.saveInfo.inventory.ammo, sBackupAmmo, sizeof(sBackupAmmo));
            sIsSwapped = false;
        }
    });

    COND_HOOK(OnKaleidoUpdate, CVAR_CUSTOM_PANEL_ENABLED, [](PauseContext* pauseCtx) {
        // Animation update
        if (sFlipAngle != sTargetFlipAngle) {
            float prevAngle = sFlipAngle;
            float step = (sTargetFlipAngle - sFlipAngle) * 0.4f;
            if (fabsf(step) < 0.001f) {
                sFlipAngle = sTargetFlipAngle;
            } else {
                sFlipAngle += step;
            }

            // Toggle data precisely when crossing the 90 degree mark (M_PI / 2)
            float midpoint = (float)M_PI / 2.0f;
            if ((prevAngle < midpoint && sFlipAngle >= midpoint) || (prevAngle > midpoint && sFlipAngle <= midpoint)) {
                sShowingCustomItems = (sTargetFlipAngle > midpoint);
            }
        }

        if (pauseCtx->state == PAUSE_STATE_MAIN && pauseCtx->pageIndex == PAUSE_ITEM &&
            pauseCtx->mainState == PAUSE_MAIN_STATE_IDLE) {

            Input* input = &gPlayState->state.input[0];
            // Use C-Up to trigger flipping
            if (CHECK_BTN_ALL(input->press.button, BTN_CUP)) {
                if (sTargetFlipAngle == 0.0f) {
                    sTargetFlipAngle = (float)M_PI;
                } else {
                    sTargetFlipAngle = 0.0f;
                }
                Audio_PlaySfx(NA_SE_SY_WIN_SCROLL_LEFT);
            }
        }
    });

    // Custom Item Usage Logic
    COND_HOOK(OnPassPlayerInputs, CVAR_CUSTOM_PANEL_ENABLED, [](Input* input) {
        if (gPlayState == NULL || gPlayState->pauseCtx.state != 0 || gPlayState->msgCtx.msgLength != 0) {
            return;
        }

        for (int i = 0; i < 4; i++) {
            if (CHECK_BTN_ALL(input->press.button, (i == 0)   ? BTN_B
                                                   : (i == 1) ? BTN_CLEFT
                                                   : (i == 2) ? BTN_CDOWN
                                                              : BTN_CRIGHT)) {
                u8 itemOnButton = gSaveContext.save.saveInfo.equips.buttonItems[0][i];
                CustomItemId customItemId = CUSTOM_ITEM_NONE;

                // Check if it's a marker
                if (IS_HUD_MARKER(itemOnButton)) {
                    customItemId = (CustomItemId)gSaveContext.save.shipSaveInfo.customEquips[i];
                }

                if (customItemId != CUSTOM_ITEM_NONE) {
                    // Iterate registry to find matching item logic
                    for (const auto& entry : sCustomItemRegistry) {
                        if (entry.customItemId == customItemId) {
                            if (entry.canUse && entry.canUse()) {
                                if (entry.onUse) {
                                    entry.onUse();
                                }
                            } else {
                                Audio_PlaySfx(NA_SE_SY_ERROR);
                            }

                            // Consume Input
                            input->press.button &= ~((i == 0)   ? BTN_B
                                                     : (i == 1) ? BTN_CLEFT
                                                     : (i == 2) ? BTN_CDOWN
                                                                : BTN_CRIGHT);
                            break;
                        }
                    }
                }
            }
        }
    });

    COND_ID_HOOK(
        BeforeKaleidoDrawPage, PAUSE_ITEM, CVAR_CUSTOM_PANEL_ENABLED, [](PauseContext* pauseCtx, u16 pauseIndex) {
            // Fix equip outline: The outline is slot-based, so it incorrectly shows on both panels if they
            // share a slot index. We temporarily set buttonItems to ITEM_NONE for the draw call if the item in
            // that slot doesn't match the equipped item.
            for (int i = 0; i < 3; i++) {
                u8 slot = gSaveContext.save.saveInfo.equips.cButtonSlots[0][i + 1];
                if (slot < ITEM_NUM_SLOTS) {
                    u8 itemInSlot = gSaveContext.save.saveInfo.inventory.items[slot];
                    u8 itemOnButton = gSaveContext.save.saveInfo.equips.buttonItems[0][i + 1];
                    bool match = (itemInSlot == itemOnButton);

                    // Decoupled marker matching
                    if (!match && IS_HUD_MARKER(itemOnButton)) {
                        u8 index = itemOnButton - ITEM_CUSTOM_MARKER_B;
                        CustomItemId customItemId = (CustomItemId)gSaveContext.save.shipSaveInfo.customEquips[index];
                        if (customItemId != CUSTOM_ITEM_NONE) {
                            for (const auto& entry : sCustomItemRegistry) {
                                if (entry.customItemId == customItemId && entry.markerId == itemInSlot) {
                                    match = true;
                                    break;
                                }
                            }
                        }
                    }

                    if (!match) {
                        sBackupButtonItems[i] = itemOnButton;
                        gSaveContext.save.saveInfo.equips.buttonItems[0][i + 1] = ITEM_NONE;
                        sHasButtonBackup = true;
                    } else {
                        sBackupButtonItems[i] = itemOnButton; // Still keep it for restore loop
                    }
                } else {
                    sBackupButtonItems[i] = gSaveContext.save.saveInfo.equips.buttonItems[0][i + 1];
                }
            }

            // D-pad support
            if (CVarGetInteger("gEnhancements.Dpad.DpadEquips", 0)) {
                for (int i = 0; i < 4; i++) {
                    u8 slot = gSaveContext.save.shipSaveInfo.dpadEquips.dpadSlots[0][i];
                    if (slot < ITEM_NUM_SLOTS) {
                        u8 itemInSlot = gSaveContext.save.saveInfo.inventory.items[slot];
                        u8 itemOnButton = gSaveContext.save.shipSaveInfo.dpadEquips.dpadItems[0][i];
                        if (itemInSlot != itemOnButton) {
                            sBackupDpadItems[i] = itemOnButton;
                            gSaveContext.save.shipSaveInfo.dpadEquips.dpadItems[0][i] = ITEM_NONE;
                            sHasDpadBackup = true;
                        } else {
                            sBackupDpadItems[i] = itemOnButton;
                        }
                    } else {
                        sBackupDpadItems[i] = gSaveContext.save.shipSaveInfo.dpadEquips.dpadItems[0][i];
                    }
                }
            }
        });

    COND_ID_HOOK(AfterKaleidoDrawPage, PAUSE_ITEM, CVAR_CUSTOM_PANEL_ENABLED,
                 [](PauseContext* pauseCtx, u16 pauseIndex) {
                     // Always try to restore backups if they exist, to prevent corruption if CVar toggled mid-draw
                     // (unlikely but safe)
                     if (sHasButtonBackup) {
                         for (int i = 0; i < 3; i++) {
                             gSaveContext.save.saveInfo.equips.buttonItems[0][i + 1] = sBackupButtonItems[i];
                         }
                         sHasButtonBackup = false;
                     }
                     if (sHasDpadBackup) {
                         for (int i = 0; i < 4; i++) {
                             gSaveContext.save.shipSaveInfo.dpadEquips.dpadItems[0][i] = sBackupDpadItems[i];
                         }
                         sHasDpadBackup = false;
                     }
                 });

    COND_ID_HOOK(BeforeKaleidoDrawPageSections, PAUSE_ITEM, CVAR_CUSTOM_PANEL_ENABLED,
                 [](PauseContext* pauseCtx, u16 pauseIndex) {
                     // Apply localized rotation to the entire entire page matrix (background and items)
                     // If we are showing custom items, we offset the angle by 180 degrees so that the "back" of the
                     // flip is facing the camera (and thus visible and correctly oriented).
                     float angle = sFlipAngle;
                     if (sShowingCustomItems) {
                         angle += (float)M_PI;

                         // Purple theme for Custom Panel
                         GraphicsContext* gfxCtx = gPlayState->state.gfxCtx;
                         OPEN_DISPS(gfxCtx);
                         gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 150, 100, 200, pauseCtx->alpha);
                         CLOSE_DISPS(gfxCtx);
                     }
                     if (angle != 0.0f) {
                         Matrix_RotateXF(angle, MTXMODE_APPLY);
                     }
                 });

    REGISTER_VB_SHOULD(VB_CHECK_ITEM_SWAP_EQUIP_SLOT, {
        ItemId targetItem = (ItemId)va_arg(args, int);
        ItemId existingItem = (ItemId)va_arg(args, int);

        if (existingItem != ITEM_NONE && targetItem != ITEM_NONE) {
            if (targetItem != existingItem) {
                *should = false;
            }
        }
    });

    REGISTER_VB_SHOULD(VB_GET_ITEM_ICON_TEXTURE, {
        u8 itemId = va_arg(args, int);
        void** texture = va_arg(args, void**);

        if (IS_CUSTOM_MARKER(itemId)) {
            *texture = (void*)gEmptyTexture; // Prevent corruption by defaulting to empty
            CustomItemId customItemId = CUSTOM_ITEM_NONE;

            // HUD markers (mapping to slot index)
            if (IS_HUD_MARKER(itemId)) {
                u8 index = itemId - ITEM_CUSTOM_MARKER_B; // 0, 1, 2, 3
                customItemId = (CustomItemId)gSaveContext.save.shipSaveInfo.customEquips[index];
            }
            // Specific item markers (Inventory)
            else if (itemId == ITEM_CUSTOM_MARKER_DINS_FIRE) {
                customItemId = CUSTOM_ITEM_DINS_FIRE;
            }

            if (customItemId != CUSTOM_ITEM_NONE) {
                for (const auto& entry : sCustomItemRegistry) {
                    if (entry.customItemId == customItemId) {
                        if (entry.getIcon) {
                            void* icon = entry.getIcon();
                            if (icon != nullptr) {
                                *texture = icon;
                            }
                        }
                        break;
                    }
                }
            }
        }
    });

    REGISTER_VB_SHOULD(VB_ITEM_HAS_NORMAL_EQUIP_ANIMATION, {
        u8 itemId = va_arg(args, int);

        // Check if this is a registered custom item marker
        if (IS_CUSTOM_MARKER(itemId)) {
            *should = true;
        }
    });

    REGISTER_VB_SHOULD(VB_ITEM_BE_RESTRICTED, {
        u8 itemId = *(u8*)va_arg(args, u8*);

        if (IS_CUSTOM_MARKER(itemId)) {
            *should = false; // Custom items are not restricted
        }
    });
    REGISTER_VB_SHOULD(VB_PLAYER_INIT_ITEM_ACTION, {
        PlayerItemAction itemAction = va_arg(args, PlayerItemAction);
        Player* player = GET_PLAYER(gPlayState);

        if (itemAction > PLAYER_IA_MAX) {
            *should = false;
            switch (itemAction) {
                case 90:
                    itemAction = PLAYER_IA_NONE;
                    break;
                default:
                    itemAction = PLAYER_IA_NONE;
                    break;
            }
            player->itemAction = player->heldItemAction = itemAction;
            player->modelGroup = player->nextModelGroup;

            player->stateFlags1 &= ~(PLAYER_STATE1_USING_ZORA_BOOMERANG | PLAYER_STATE1_8);

            player->unk_B08 = 0.0f;
            player->unk_B0C = 0.0f;
            player->unk_B28 = 0;

            // Player_InitDefaultIA;
            // Player_SetModelGroup(player, (PlayerModelGroup)player->modelGroup);
        }
    });
}

static RegisterShipInitFunc initFunc(RegisterCustomItemPanel, { CVAR_CUSTOM_PANEL_ENABLED_NAME });
