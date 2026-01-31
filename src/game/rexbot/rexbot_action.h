#pragma once

#include "godot_cpp/core/math_defs.hpp"
#include "rexbot_action_result.h"

using namespace godot;

class RexbotNPCBase;

class RexbotAction {
    RexbotNPCBase *actor = nullptr;

    RexbotAction *action_buried_under_me = nullptr;
    RexbotAction *action_covering_me = nullptr;
protected:
    [[nodiscard]] RexbotActionResult action_continue() {
        return RexbotActionResult {
            .result_type = RexbotActionResult::ResultType::CONTINUE
        };
    }

    [[nodiscard]] RexbotActionResult suspend_for(RexbotAction *p_action, const char *p_reason) {
        return RexbotActionResult {
            .result_type = RexbotActionResult::ResultType::SUSPEND_FOR,
            .reason = p_reason,
            .action = p_action
        }; 
    }
    [[nodiscard]] RexbotActionResult change_to(RexbotAction *p_action, const char *p_reason) {
        return RexbotActionResult {
            .result_type = RexbotActionResult::ResultType::CHANGE_TO,
            .reason = p_reason,
            .action = p_action
        }; 
    }

    [[nodiscard]] RexbotActionResult done(const char *p_reason) {
        return RexbotActionResult {
            .result_type = RexbotActionResult::ResultType::DONE,
            .reason = p_reason
        }; 
    }

    virtual RexbotActionResult on_start() { return action_continue(); }
    virtual RexbotActionResult on_end() { return action_continue(); }
    virtual RexbotActionResult on_suspend() { return action_continue(); }
    virtual RexbotActionResult on_resume() { return action_continue(); }
    virtual RexbotActionResult update(real_t p_delta) = 0;

public:

    RexbotNPCBase *get_actor() const;
    RexbotAction(RexbotNPCBase *p_actor);

    virtual const char* get_name() const = 0;

    friend class RexbotIntention;
    friend class RexbotBehaviour;
};