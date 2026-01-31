#include "npc_turret_behaviour.h"
#include "game/rexbot/rexbot_behaviour.h"
#include "game/turret/npc_turret_patrol_action.h"
#include "npc_turret.h"

RexbotAction *NPCTurretBehaviour::get_initial_action() const {
    return memnew(NPCTurretPatrolAction(turret));
}

NPCTurretBehaviour::NPCTurretBehaviour(NPCTurret *p_turret) : RexbotBehaviour(p_turret) {
    turret = p_turret;
}
