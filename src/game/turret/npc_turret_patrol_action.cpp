#include "npc_turret_patrol_action.h"

#include "game/rexbot/rexbot_brain.h"
#include "game/rexbot/rexbot_vision.h"
#include "game/turret/npc_turret_attack_action.h"
#include "npc_turret.h"

const char *NPCTurretPatrolAction::get_name() const {
	return "TurretPatrol";
}

RexbotActionResult NPCTurretPatrolAction::update(real_t p_delta) {
	RexbotBrain *brain = get_actor()->get_brain();
	RexbotVision *vision = brain->get_vision();
	if (int threat_idx = vision->get_primary_threat_idx(); threat_idx != -1) {
		if (vision->get_known_actor_character(threat_idx) != nullptr) {
			return suspend_for(memnew(NPCTurretAttackAction(get_actor())), "Enemy spotted!");
		}
	}

	return action_continue();
}

NPCTurretPatrolAction::NPCTurretPatrolAction(NPCTurret *p_turret) :
		RexbotAction(p_turret) {
}
