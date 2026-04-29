#include "npc_turret_attack_action.h"
#include "game/base_character.h"
#include "game/rexbot/rexbot_npc_base.h"
#include "game/rexbot/rexbot_vision.h"
#include "game/rexbot/rexbot_brain.h"
#include "godot_cpp/classes/control.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

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
        return done("Threat lost");
    }

    BaseCharacter *threat_actor = vision->get_known_actor_character(primary_threat_idx);
    if (threat_actor == nullptr) {
        return done("Threat actor is invalid, bug?");
    }

    static const Vector3 FORWARD = Vector3(0.0, 0.0, -1.0f);
    actor->get_brain()->get_vision()->aim_head_to_character(threat_actor, RexbotVision::LookAtTargetPriority::HIGH, 0.1f, "Look at threat");

    if (actor->get_brain()->get_vision()->is_aiming_at_target() && !actor->is_action_pressed(BaseCharacter::InputCommand::PRIMARY_FIRE)) {
        if (UtilityFunctions::randf() > 0.33f) {
            actor->press_primary_fire();
        }
    }

    return action_continue();
}
