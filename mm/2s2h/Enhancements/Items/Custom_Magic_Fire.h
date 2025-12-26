#ifndef COMBO_OVL_MM_CUSTOM_MAGIC_FIRE_H
#define COMBO_OVL_MM_CUSTOM_MAGIC_FIRE_H

#include "align_asset_macro.h"

#define dgFireTex "__OTR__overlays/ovl_Magic_Fire/gFireTex"
static const ALIGN_ASSET(2) char gFireTex[] = dgFireTex;

#define dgDinsFireIcon "__OTR__overlays/ovl_Magic_Fire/gItemIconDinsFireTex"
static const ALIGN_ASSET(2) char gDinsFireIcon[] = dgDinsFireIcon;

typedef struct {
    Actor actor;
    ColliderCylinder collider;
    float alphaMultiplier;
    float screenTintIntensity;
    float scalingSpeed;
    s16 action;
    s16 screenTintBehaviour;
    s16 actionTimer;
    s16 screenTintBehaviourTimer;
} Actor_CustomMagicFire;

#endif
