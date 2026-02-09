#include <libultraship/bridge/consolevariablebridge.h>
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"

extern "C" {
#include "functions.h"
#include "variables.h"
#include "overlays/actors/ovl_En_Arrow/z_en_arrow.h"
#include "overlays/actors/ovl_En_Bom/z_en_bom.h"
extern s32 sPlayerUseHeldItem;
}

#define CVAR_NAME "gEnhancements.PlayerActions.BombArrows"
#define CVAR CVarGetInteger(CVAR_NAME, 0)

static bool ActorExistsInCategory(PlayState* play, Actor* actor, u8 category) {
    if (play == NULL || actor == NULL) {
        return false;
    }

    Actor* current = play->actorCtx.actorLists[category].first;
    while (current != NULL) {
        if (current == actor) {
            return true;
        }
        current = current->next;
    }
    return false;
}

static void BombArrows_HandleAfterDraw(PlayState* play, EnArrow* arrow) {
    if (arrow->actor.child == NULL || arrow->actor.child->id != ACTOR_EN_BOM) {
        return;
    }

    EnBom* bomb = (EnBom*)arrow->actor.child;
    if (bomb->timer != 0 && bomb->actor.params == BOMB_TYPE_BODY) {
        Vec3f tip;
        Vec3f temp = { 64.0f, -64.0f, 1000.0f };
        Matrix_MultVec3f(&temp, &tip);
        f32 yDiff = tip.y - bomb->actor.world.pos.y;
        Math_Vec3f_Copy(&bomb->actor.world.pos, &tip);
        f32 projectedWaterSurfaceDist = bomb->actor.depthInWater - yDiff;

        if (projectedWaterSurfaceDist >= 20.0f) {
            arrow->collider.elem.atDmgInfo.dmgFlags = 0x20;
            bomb->actor.parent = NULL;
            arrow->actor.child = NULL;
            Actor_SetScale(&bomb->actor, 0.01f);
        }
    }
}

static bool BombArrows_HandleHit(PlayState* play, EnArrow* arrow, bool hitActor) {
    if (arrow->actor.child == NULL || arrow->actor.child->id != ACTOR_EN_BOM) {
        return true;
    }

    if (!(arrow->unk_262 || hitActor)) {
        return true;
    }

    EnBom* bomb = (EnBom*)arrow->actor.child;
    if (hitActor) {
        Math_Vec3s_ToVec3f(&bomb->actor.world.pos, &arrow->collider.elem.acDmgInfo.hitPos);
    } else {
        Math_Vec3f_Copy(&bomb->actor.world.pos, &arrow->actor.world.pos);
    }

    bomb->actor.parent = NULL;
    arrow->actor.child = NULL;
    Actor_Kill(&arrow->actor);

    bomb->timer = 0;
    arrow->unk_262 = 0;
    return false;
}

static bool BombArrows_HandlePlayerUse(PlayState* play, Player* player, ItemId item) {
    if (item != ITEM_BOMB) {
        return false;
    }

    if (player->heldActor == NULL || player->heldActor->id != ACTOR_EN_ARROW) {
        return false;
    }

    EnArrow* arrow = (EnArrow*)player->heldActor;
    if (arrow->actor.params != ARROW_TYPE_NORMAL) {
        return false;
    }

    if (arrow->actor.child == NULL) {
        EnBom* bomb = (EnBom*)Actor_SpawnAsChild(&play->actorCtx, &arrow->actor, play, ACTOR_EN_BOM,
                                                 arrow->actor.world.pos.x, arrow->actor.world.pos.y,
                                                 arrow->actor.world.pos.z, 0, arrow->actor.shape.rot.y, 0, 0);
        if (bomb != NULL) {
            bomb->collider1.base.acFlags &= ~AC_TYPE_PLAYER;
            arrow->collider.elem.atDmgInfo.dmgFlags = 8;
            Actor_SetScale(&bomb->actor, 0.002f);
            Inventory_ChangeAmmo(item, -1);
        }
    }

    sPlayerUseHeldItem = true;
    return true;
}

void RegisterBombArrows() {
    COND_HOOK(ShouldPlayerUseHeldItem, CVAR, [](PlayState* play, Player* player, ItemId item, s32 actionParam,
                                                bool* should) {
        (void)actionParam;
        if (!*should) {
            return;
        }

        if (BombArrows_HandlePlayerUse(play, player, item)) {
            *should = false;
        }
    });

    COND_HOOK(OnArrowAfterDraw, CVAR,
              [](PlayState* play, EnArrow* arrow) { BombArrows_HandleAfterDraw(play, arrow); });

    COND_HOOK(ShouldArrowHit, CVAR, [](PlayState* play, EnArrow* arrow, bool hitActor, bool* should) {
        if (!*should) {
            return;
        }

        if (!BombArrows_HandleHit(play, arrow, hitActor)) {
            *should = false;
        }
    });

    COND_HOOK(OnBombUpdateWithParent, CVAR, [](PlayState* play, EnBom* bomb) {
        if (bomb->actor.parent == NULL || bomb->actor.parent->id != ACTOR_EN_ARROW) {
            return;
        }

        if (ActorExistsInCategory(play, bomb->actor.parent, ACTORCAT_ITEMACTION)) {
            if (bomb->timer == 67) {
                Actor_SetScale(&bomb->actor, 0.002f);
            }
        } else {
            bomb->actor.parent = NULL;
            Actor_SetScale(&bomb->actor, 0.01f);
        }
    });
}

static RegisterShipInitFunc initFunc(RegisterBombArrows, { CVAR_NAME });
