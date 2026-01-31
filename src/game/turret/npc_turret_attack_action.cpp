#include "npc_turret_attack_action.h"
#include "game/rexbot/rexbot_npc_base.h"
#include "game/rexbot/rexbot_vision.h"
#include "game/rexbot/rexbot_brain.h"

NPCTurretAttackAction::NPCTurretAttackAction(RexbotNPCBase *p_actor)
    : RexbotAction(p_actor)
{
    
}

const char *NPCTurretAttackAction::get_name() const {
    return "NPCTurretAttackAction";
}

RexbotActionResult NPCTurretAttackAction::update(real_t p_delta) {
    RexbotNPCBase *actor = get_actor();
    RexbotVision *vision = actor->get_brain()->get_vision();
    int primary_threat_idx = vision->get_primary_threat_idx();
    if (primary_threat_idx == -1) {
        return done("No more threats!");
    }

    BaseCharacter *threat_actor = vision->get_known_actor_character(primary_threat_idx);
    if (threat_actor == nullptr) {
        return done("Threat actor is invalid, bug?");
    }

    return action_continue();
}
