#include "rexbot_npc_base.h"

#include "bind_macros.h"
#include "debug/debug_constexpr.h"
#include "debug/debug_overlay.h"
#include "gdextension_interface.h"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/input.hpp"
#include "rexbot_behaviour.h"
#include "rexbot_brain.h"

CVar RexbotNPCBase::rexbot_debug_behaviour_cvar = CVar::create_variable("rb.debug_behaviour", GDEXTENSION_VARIANT_TYPE_BOOL, false, "Whether or not to show current actions on top of Rexbot NPCs");
CVar RexbotNPCBase::rexbot_debug_vision_cvar = CVar::create_variable("rb.debug_vision", GDEXTENSION_VARIANT_TYPE_BOOL, false, "Whether or not to show view information on top of Rexbot NPCs");
CVar RexbotNPCBase::rexbot_debug_health_cvar = CVar::create_variable("rb.debug_health", GDEXTENSION_VARIANT_TYPE_BOOL, false, "Whether or not to show health information on top of Rexbot NPCs");

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

	for (NPCButtonInputState &input : input_state.buttons) {
		if (input.time_left > 0.0f) { // are we using duration?
			input.time_left -= p_delta;

			if (input.time_left <= 0.0) {
				input.state = 0;
				input.state.set_flag(InputActionState::JUST_RELEASED);
				continue; // We'll clear this the next frame
			}
		}

		// handle clearing JUST_RELEASED
		if (input.state.has_flag(InputActionState::JUST_RELEASED)) {
			input.state = 0;
		}
	}

	BaseCharacter::_physics_process(p_delta);

	brain->update(p_delta);
	behaviour->update(p_delta);

	Vector3 planar_lookat = get_look_direction();
	planar_lookat.y = 0.0f;
	planar_lookat.normalize();
	if (planar_lookat.is_normalized()) {
		get_model()->set_target_facing_direction(planar_lookat);
	}

	if constexpr (!Debug::is_debug_enabled) {
		return;
	}

	PackedStringArray debug_strings;
	if (rexbot_debug_health_cvar.get_bool()) {
		debug_strings.push_back(vformat("Health: %d", character_state.health));
	}

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

Vector2 RexbotNPCBase::get_movement_vector() const {
	return input_state.movement_vector;
}

Vector2 RexbotNPCBase::get_movement_vector_transformed() const {
	return input_state.movement_vector;
}

BitField<BaseCharacter::InputActionState> RexbotNPCBase::get_action_state(InputCommand p_action) const {
	ERR_FAIL_INDEX_V(p_action, input_state.buttons.size(), 0);
	return input_state.buttons[p_action].state;
}

void RexbotNPCBase::get_aim_trajectory(int p_weapon_slot, Vector3 &r_origin, Vector3 &r_direction) {
	r_origin = get_model()->get_eye_position();
	r_direction = get_look_direction();
}

Vector3 RexbotNPCBase::get_look_direction() const {
	return look_direction;
}

void RexbotNPCBase::set_look_direction(const Vector3 &p_look_direction) {
	look_direction = p_look_direction;
}

void RexbotNPCBase::press_primary_fire(float p_duration) {
	NPCButtonInputState &button_state = input_state.buttons[InputCommand::PRIMARY_FIRE];
	button_state.state = 0;
	const bool was_pressed = button_state.state.has_flag(InputActionState::PRESSED);
	button_state.state.set_flag(InputActionState::PRESSED);
	if (!was_pressed) {
		button_state.state.set_flag(InputActionState::JUST_PRESSED);
		button_state.time_left = MAX(p_duration, 0.1f);
	}
}

RexbotNPCBase::~RexbotNPCBase() {
	if (brain) {
		memdelete(brain);
	}

	if (behaviour) {
		memdelete(behaviour);
	}
}
