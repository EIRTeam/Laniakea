#include "rexbot_npc_base.h"
#include "bind_macros.h"
#include "debug/debug_constexpr.h"
#include "debug/debug_overlay.h"
#include "gdextension_interface.h"
#include "rexbot_behaviour.h"
#include "godot_cpp/classes/engine.hpp"
#include "rexbot_brain.h"

CVar RexbotNPCBase::rexbot_debug_behaviour_cvar = CVar::create_variable("rb.debug_behaviour", GDEXTENSION_VARIANT_TYPE_BOOL, false, "Whether or not to show current actions on top of Rexbot NPCs");
CVar RexbotNPCBase::rexbot_debug_vision_cvar = CVar::create_variable("rb.debug_vision", GDEXTENSION_VARIANT_TYPE_BOOL, false, "Whether or not to show view information on top of Rexbot NPCs");

void RexbotNPCBase::_bind_methods() {
    MAKE_BIND_RESOURCE(RexbotNPCBase, configuration, RexbotConfiguration);
}

void RexbotNPCBase::_ready() {
    if (Engine::get_singleton()->is_editor_hint()) {
        return;
    }

    BaseCharacter::_ready();

    DEV_ASSERT(configuration.is_valid());
    brain = memnew(RexbotBrain(this, configuration));
    behaviour = create_starting_behaviour();
    behaviour->initialize();
}

void RexbotNPCBase::_physics_process(double p_delta) {
    if (Engine::get_singleton()->is_editor_hint()) {
        return;
    }
    BaseCharacter::_physics_process(p_delta);

    brain->update(p_delta);
    behaviour->update(p_delta);
    
    
    if constexpr (!Debug::is_debug_enabled) {
        return;
    }

    PackedStringArray debug_strings;
    if (rexbot_debug_behaviour_cvar.get_bool()) {
        debug_strings.push_back(behaviour->get_debug_string());
    }

    if (rexbot_debug_vision_cvar.get_bool()) {
        debug_strings.push_back(brain->get_vision()->get_debug_string());
    }

    if (!debug_strings.is_empty()) {
        DebugOverlay::text(get_global_position(), String("\n").join(debug_strings), Color(1.0, 0.0, 0.0), false);
    }
}

RexbotBrain *RexbotNPCBase::get_brain() const {
    return brain;
}

RexbotNPCBase::~RexbotNPCBase() {
    if (brain) {
        memdelete(brain);
    }

    if (behaviour) {
        memdelete(behaviour);
    }
}
