#pragma once

#include "game/rexbot/rexbot_behaviour.h"

class NPCTurret;
class NPCTurretBehaviour : public RexbotBehaviour {
    NPCTurret *turret = nullptr;
public:
    virtual RexbotAction *get_initial_action() const override;
    NPCTurretBehaviour(NPCTurret *p_turret);
};