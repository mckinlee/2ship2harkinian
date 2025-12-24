#ifndef COMBO_OVL_MM_CUSOM_MAGIC_FIRE_H
#define COMBO_OVL_MM_CUSOM_MAGIC_FIRE_H

#define CUSTOM_KEEP_MAGIC_FIRE_TEXTURE 0x3660

typedef struct
{
    Actor               actor;
    ColliderCylinder    collider;
    float               alphaMultiplier;
    float               screenTintIntensity;
    float               scalingSpeed;
    s16                 action;
    s16                 screenTintBehaviour;
    s16                 actionTimer;
    s16                 screenTintBehaviourTimer;
}
Actor_CustomMagicFire;

#endif
