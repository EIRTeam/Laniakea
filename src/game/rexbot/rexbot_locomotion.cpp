#include "rexbot_locomotion.h"

#include "game/movement_settings.h"
#include "godot_cpp/classes/navigation_agent3d.hpp"
#include "rexbot_configuration.h"

RexbotLocomotion::RexbotLocomotion(RexbotNPCBase *p_npc, Ref<RexbotConfiguration> p_config) {
	npc = p_npc;
	config = p_config;
	movement_config = p_npc->get_movement_settings();
	DEV_ASSERT(movement_config.is_valid());
	navigation_agent = memnew(NavigationAgent3D);
	navigation_agent->set_radius(movement_config->get_radius());
	navigation_agent->set_height(movement_config->get_stance_height(0));
	p_npc->add_child(navigation_agent);
}
