#include "rexbot_behaviour.h"
#include "godot_cpp/core/memory.hpp"
#include "godot_cpp/variant/packed_string_array.hpp"
#include "rexbot_action.h"

void RexbotBehaviour::apply_action_result(const RexbotActionResult &p_result) {
    switch (p_result.result_type) {
		case RexbotActionResult::DONE: {
            DEV_ASSERT(current_action->action_buried_under_me != nullptr);
            RexbotAction *old_action = current_action;
            current_action = current_action->action_buried_under_me;
            current_action->action_covering_me = nullptr;
            memdelete(old_action);
            current_action->on_resume();
        } break;
		case RexbotActionResult::CONTINUE: {
            // nothing...
        } break;
		case RexbotActionResult::SUSPEND_FOR: {
            current_action->on_suspend();
            current_action->action_covering_me = p_result.action;
            p_result.action->action_buried_under_me = current_action;
            current_action = p_result.action;
        } break;
		case RexbotActionResult::CHANGE_TO: {
            current_action->on_end();
            p_result.action->action_buried_under_me = current_action->action_buried_under_me;
            memdelete(current_action);
            current_action = p_result.action;
            current_action->on_start();
        } break;
	}
}
void RexbotBehaviour::initialize() {
    current_action = get_initial_action();
}
void RexbotBehaviour::update(real_t p_delta) {
    if (!current_action) {
        return;
    }

    apply_action_result(current_action->update(p_delta));
};

String RexbotBehaviour::get_debug_string() const {
    PackedStringArray action_names;

    RexbotAction *action = current_action;
    while (action != nullptr) {
        action_names.append(action->get_name());
        action = action->action_buried_under_me;
    }

    action_names.reverse();

    return String(" > ").join(action_names);
}

RexbotBehaviour::~RexbotBehaviour() {
    RexbotAction *bottom_action = current_action;
    if (bottom_action != nullptr) {
        while (bottom_action->action_buried_under_me != nullptr) {
            bottom_action = bottom_action->action_buried_under_me;
        }

        while (bottom_action != nullptr) {
            RexbotAction *action = bottom_action;
            bottom_action = action->action_covering_me;
            memdelete(action);
        }
    }
}
