#pragma once

#include "game/rexbot/rexbot_action.h"

class NPCTurret;

class NPCTurretPatrolAction : public RexbotAction {
    virtual const char *get_name() const override;
    virtual RexbotActionResult update(real_t p_delta) override;
public:
    NPCTurretPatrolAction(NPCTurret *p_turret);
};