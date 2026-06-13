#include "rexbot_brain.h"

#include "rexbot_configuration.h"

void RexbotBrain::update(double p_delta) {
	vision.update(p_delta);
}

RexbotVision *RexbotBrain::get_vision() const {
	return (RexbotVision *)&vision;
}

RexbotLocomotion *RexbotBrain::get_locomotion() const {
	return (RexbotLocomotion *)&locomotion;
}

RexbotBrain::RexbotBrain(RexbotNPCBase *p_actor, Ref<RexbotConfiguration> p_config) :
		vision(p_actor, p_config), locomotion(p_actor, p_config) {
}
