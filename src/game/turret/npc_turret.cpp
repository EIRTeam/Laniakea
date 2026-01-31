#include "npc_turret.h"
#include "game/biped_animation_base.h"
#include "game/character_animation_base.h"
#include "game/turret/npc_turret_behaviour.h"
void NPCTurret::_bind_methods() {    
}

RexbotBehaviour *NPCTurret::create_starting_behaviour()
{
    return memnew(NPCTurretBehaviour(this));
}

CharacterAnimationBase *NPCTurret::create_animation() const {
    return memnew(BipedAnimationBase);
}
