#pragma once

#include "console/console_system.h"
#include "console/cvar.h"
#include "game/rexbot/rexbot_action.h"
#include "game/rexbot/rexbot_action_result.h"
#include "godot_cpp/variant/string.hpp"

class RexbotNPCBase;

class RexbotBehaviour {
	RexbotAction *current_action = nullptr;
	RexbotNPCBase *actor = nullptr;

	virtual RexbotAction *get_initial_action() const = 0;

	void apply_action_result(const RexbotActionResult &p_result);

public:
	void initialize();
	virtual void update(real_t p_delta);
	RexbotBehaviour(RexbotNPCBase *p_actor) {
		actor = p_actor;
	}
	String get_debug_string() const;
	virtual ~RexbotBehaviour();
};
