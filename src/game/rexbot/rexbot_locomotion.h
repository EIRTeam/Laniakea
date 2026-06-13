#pragma once

#include "game/rexbot/rexbot_npc_base.h"

class RexbotConfiguration;
class MovementSettings;

namespace godot {
class NavigationAgent3D;
}

class RexbotLocomotion {
	RexbotNPCBase *npc;
	NavigationAgent3D *navigation_agent = nullptr;
	Ref<RexbotConfiguration> config;
	Ref<MovementSettings> movement_config;

	enum NavigationStatus {
		FINISHED,
		NAVIGATING
	};

	struct RexbotNavigation {
		RID navigation_agent;
	} navigation;

public:
	RexbotLocomotion(RexbotNPCBase *p_npc, Ref<RexbotConfiguration> p_config);
};
