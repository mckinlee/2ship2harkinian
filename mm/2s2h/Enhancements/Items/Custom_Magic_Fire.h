#ifndef COMBO_OVL_MM_CUSOM_MAGIC_FIRE_H
#define COMBO_OVL_MM_CUSOM_MAGIC_FIRE_H

extern u64 CUSTOM_KEEP_MAGIC_FIRE_TEXTURE[];
#define ITEM_CUSTOM_DINS_FIRE ITEM_C1

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
