#pragma once

#include "game/rexbot/rexbot_action.h"

class NPCTurretAttackAction : public RexbotAction {
public:
	NPCTurretAttackAction(RexbotNPCBase *p_actor);
	virtual const char *get_name() const override;
	virtual RexbotActionResult update(real_t p_delta) override;
};
