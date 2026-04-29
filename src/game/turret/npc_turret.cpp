#include "npc_turret.h"
#include "game/biped_animation_base.h"
#include "game/character_animation_base.h"
#include "game/rexbot/rexbot_npc_base.h"
#include "game/turret/npc_turret_behaviour.h"
#include "game/weapon_rifle_test.h"

#include "godot_cpp/classes/engine.hpp"

void NPCTurret::_bind_methods() {    
}

void NPCTurret::_ready() {
    if (Engine::get_singleton()->is_editor_hint()) {
        return;
    }
    RexbotNPCBase::_ready();

    biped_anim = dynamic_cast<BipedAnimationBase*>(animation);

    DEV_ASSERT(biped_anim != nullptr);

    biped_anim->set_weapon_animation_type(BipedAnimationBase::WEAPON_ANIMATION_TYPE_RIFLE);
    biped_anim->set_is_aiming(true);

    Ref<WeaponRifleTest> rifle_test;
    rifle_test.instantiate();
    equip_weapon(WEAPON_SLOT_PRIMARY, rifle_test);
}

RexbotBehaviour *NPCTurret::create_starting_behaviour()
{
    return memnew(NPCTurretBehaviour(this));
}

CharacterAnimationBase *NPCTurret::create_animation() const {
    return memnew(BipedAnimationBase);
}
