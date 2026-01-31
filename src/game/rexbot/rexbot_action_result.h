#pragma once

class RexbotAction;

struct RexbotActionResult {
    enum ResultType {
        DONE,
        CONTINUE,
        SUSPEND_FOR,
        CHANGE_TO
    };

    ResultType result_type;
    const char* reason = nullptr;
    RexbotAction *action = nullptr;
};