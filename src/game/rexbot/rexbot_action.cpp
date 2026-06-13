#include "rexbot_action.h"
RexbotNPCBase *RexbotAction::get_actor() const {
	return actor;
}

RexbotAction::RexbotAction(RexbotNPCBase *p_actor) :
		actor(p_actor) {
}
