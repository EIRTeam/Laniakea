#include "vehicle.h"
#include "../physics.h"
#include "bind_macros.h"
#include "debug/debug_overlay.h"
#include "godot_cpp/classes/control.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/file_access.hpp"
#include "godot_cpp/classes/physics_direct_space_state3d.hpp"
#include "godot_cpp/classes/physics_ray_query_parameters3d.hpp"
#include "godot_cpp/classes/rigid_body3d.hpp"
#include "godot_cpp/classes/world3d.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/math_defs.hpp"
#include "godot_cpp/variant/string_name.hpp"
#include "math.h"
#include "vehicle/shaft.h"
#include "vehicle/steering_rack.h"
#include "vehicle/vehicle_drivetrain_debugger.h"
#include "vehicle/vehicle_wheel_shaft.h"
#include "vehicle/wheel_position.h"
#include "vehicle_suspension_settings.h"
#include "vehicle_wheel.h"
#include "vehicle_wheel_settings.h"
#include <cfenv>
#include <queue>

void LNVehicle::_apply_arb(int p_wheel_left, int p_wheel_right, float p_arb_stiffness) {
	/*WheelData &left  = wheels[p_wheel_left];
	WheelData &right = wheels[p_wheel_right];

	if (!left.hit || !right.hit) return;

	const float arb_torque = p_arb_stiffness
						   * (left.spring_displacement - right.spring_displacement);

	// Use contact normals, consistent with spring force
	_apply_force(-left.contact_normal  * arb_torque,
			left.hit_position  - get_global_position(), Color(1.0, 0.0, 0.0));
	_apply_force( right.contact_normal * arb_torque,
			right.hit_position - get_global_position(), Color(1.0, 0.0, 0.0));*/
}

void LNVehicle::_apply_force(Vector3 p_force_global, Vector3 p_offset_global, std::optional<Color> p_color) {
	DEV_ASSERT(p_force_global.is_finite());
	if (const Vector3 force_visual = p_force_global / 500.0f; !force_visual.is_zero_approx()) {
		Color color = p_color.value_or(Color(0.0, 1.0, 1.0));
		DebugOverlay::filled_arrow(get_global_position() + p_offset_global, get_global_position() + p_offset_global + force_visual, 0.15f, color, false);
	}
	apply_force(p_force_global, p_offset_global);
}

void LNVehicle::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_brake_percentage", "brake"), &LNVehicle::set_brake_percentage);
	ClassDB::bind_method(D_METHOD("set_steer_percentage", "steer"), &LNVehicle::set_steer_percentage);
	ClassDB::bind_method(D_METHOD("set_clutch_percentage", "clutch"), &LNVehicle::set_clutch_percentage);
	ClassDB::bind_method(D_METHOD("set_throttle_percentage", "throttle"), &LNVehicle::set_throttle_percentage);
	ClassDB::bind_method(D_METHOD("request_gear_up"), &LNVehicle::request_gear_up);
	ClassDB::bind_method(D_METHOD("request_gear_down"), &LNVehicle::request_gear_down);
	ClassDB::bind_method(D_METHOD("get_engine_torque"), &LNVehicle::get_engine_torque);
	ClassDB::bind_method(D_METHOD("get_current_gear"), &LNVehicle::get_current_gear);
	ClassDB::bind_method(D_METHOD("get_wheel_slip_angle"), &LNVehicle::get_wheel_slip_angle);
	ClassDB::bind_method(D_METHOD("get_wheel_slip_ratio"), &LNVehicle::get_wheel_slip_ratio);
	ClassDB::bind_method(D_METHOD("get_engine_rpm"), &LNVehicle::get_engine_rpm);
	MAKE_BIND_NODE(LNVehicle, audio_stream_player, AudioStreamPlayer);
	MAKE_BIND_RESOURCE(LNVehicle, vehicle_settings, LNVehicleSettings);

	BIND_ENUM_CONSTANT(WHEEL_FL);
	BIND_ENUM_CONSTANT(WHEEL_RL);
	BIND_ENUM_CONSTANT(WHEEL_RR);
	BIND_ENUM_CONSTANT(WHEEL_FR);
}

/*Vector2 brush_gdsim(Vector2 p_slip, float p_contact_patch, float p_coefficient_of_friction, float p_tire_stiffness, float p_y_force) {
	// float con_patch = 0.35f;
	// float mu = 0.85f;
	// float tire_stiffness = 5.5f;
	float con_patch = p_contact_patch;
	float mu = p_coefficient_of_friction;
	float tire_stiffness = p_tire_stiffness;

	// 500000 because this line is multiplied by 0.5, while stiffness values are actually in the millions
	// tire_stiffness is a small value just for convenience
	float stiffness = 500000.0f * tire_stiffness * Math::pow(con_patch, 2.0f);
	float friction = mu * p_y_force;
	// "Brush" tire formula
	float deflect = Math::sqrt(Math::pow(stiffness * p_slip.y, 2.0f) + Math::pow(stiffness * Math::tan(p_slip.x), 2.0f));
	if (deflect == 0.0f || !Math::is_finite(deflect)) {
		return Vector2();
	} else {
		Vector2 vector = Vector2();
		float crit_length = friction * (1 - p_slip.y) * con_patch / (2 * deflect);
		if (crit_length >= con_patch) {
			vector.y = stiffness * -p_slip.y / (1.0f - p_slip.y);
			vector.x = stiffness * Math::tan(p_slip.x) / (1 - p_slip.y);
		} else {
			float brushy = (1.0f - friction * (1 - p_slip.y) / (4.0f * deflect)) / deflect;
			vector.y = friction * stiffness * -p_slip.y * brushy;
			vector.x = friction * stiffness * Math::tan(p_slip.x) * brushy;
		}
		DEV_ASSERT(vector.is_finite());
		return vector;
	}
}*/

Vector2 brush(Vector2 slip, Vector2 p_stiffness, float friction, float load) {
	float deflection = Math::sqrt(Math::pow(p_stiffness.y * slip.y, 2) +
			Math::pow(p_stiffness.x * slip.x, 2));

	if (deflection == 0.0f)
		return { 0.0f, 0.0f };

	float crit_limit = friction * load * 0.5f;

	Vector2 force;
	if (deflection <= crit_limit) {
		// Adhesion Region: Linear relationship
		force.x = p_stiffness.x * slip.x;
		force.y = p_stiffness.y * slip.y;
	} else {
		// Sliding Region: Non-linear brush decay
		float brush_factor = (1.0f - (crit_limit / (4.0f * deflection))) / deflection;
		float friction_force = friction * load;

		force.x = friction_force * (p_stiffness.x * slip.x) * brush_factor;
		force.y = friction_force * (p_stiffness.y * slip.y) * brush_factor;
	}

	return force;
}

void LNVehicle::_physics_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint()) {
		_debug_draw();
		return;
	}

	// First of all, update from bottom up
	LocalVector<Ref<LNVehicleShaft>> leaves;
	LocalVector<Ref<LNVehicleShaft>> roots;

	for (KeyValue<StringName, Ref<LNVehicleShaft>> shaft : shafts) {
		shaft.value->pre_update(p_delta, input_state);

		if (!shaft.value->has_input()) {
			roots.push_back(shaft.value);
		}

		if (shaft.value->get_output_count() == 0) {
			leaves.push_back(shaft.value);
		}
	}

	// There should only be one root!
	DEV_ASSERT(roots.size() <= 1);

	// Wheels are special, they need to calculate the reaction torque and suspension forces beforehand

	const Transform3D suspension_trf_left = wheels[WHEEL_FL].shaft->get_suspension_transform(this, wheels[WHEEL_FL].wheel);
	const Transform3D suspension_trf_right = wheels[WHEEL_FR].shaft->get_suspension_transform(this, wheels[WHEEL_FR].wheel);
	const LNVehicleSteeringRack::SteeringRackResult steering_rack_result = LNVehicleSteeringRack::solve(
			vehicle_settings,
			{ wheels[WHEEL_FL].wheel->get_suspension_settings(), wheels[WHEEL_FR].wheel->get_suspension_settings() },
			{ suspension_trf_left, suspension_trf_right }, input_state.steer);
	for (const WheelData &wheel_data : wheels) {
		Vector3 rack_pos = wheel_data.wheel->get_wheel_position() == WHEEL_FL ? steering_rack_result.left_position : steering_rack_result.right_position;
		wheel_data.shaft->wheel_pre_update(p_delta, rack_pos, input_state, this, wheel_data.wheel);
	}

	// Wheels are updated, let's now propagate everything upstream
	// roots take care of propagating and balancing the torque and angular velocity of their children
	for (Ref<LNVehicleShaft> root : roots) {
		std::queue<LNVehicleShaft *> update_queue;
		update_queue.push(root.ptr());

		while (!update_queue.empty()) {
			update_queue.front()->update(p_delta, input_state);
			for (int i = 0; i < update_queue.front()->get_output_count(); i++) {
				if (LNVehicleShaft *child = update_queue.front()->get_child(i); child != nullptr) {
					update_queue.push(child);
				}
			}
			update_queue.pop();
		}
	}

	// Post-update wheels, this will integrate the wheel drivetrain components
	for (const WheelData &wheel_data : wheels) {
		wheel_data.shaft->wheel_post_update(p_delta, input_state, this, wheel_data.wheel);
	}

	// Finally, apply ARBs
	wheels[WHEEL_FL].shaft->apply_arb(this, wheels[WHEEL_FR].shaft, vehicle_settings->get_front_arb_stiffness());
	wheels[WHEEL_RL].shaft->apply_arb(this, wheels[WHEEL_RR].shaft, vehicle_settings->get_rear_arb_stiffness());

	debugger->update();
}

void LNVehicle::_debug_draw() {
	/*for (int wheel_idx = 0; wheel_idx < WHEEL_MAX; wheel_idx++) {
		WheelData &wheel_data = wheels[wheel_idx];
		LNVehicleWheel *wheel = wheels[wheel_idx].wheel;

		if (wheel == nullptr) {
			continue;
		}

		Ref<LNVehicleSuspensionSettings> suspension_settings = wheel->get_suspension_settings();
		Ref<LNVehicleWheelSettings> wheel_settings = wheel->get_wheel_settings();
		if (suspension_settings.is_null() || wheel_settings.is_null()) {
			continue;
		}

		const Vector3 world_attachment_point = to_global(wheel->get_top_attachment_point());
		const Vector3 world_wheel_direction = get_global_basis().xform(Vector3(0.0, -1.0, 0.0));

		if (Engine::get_singleton()->is_editor_hint()) {
			DebugOverlay::filled_arrow(world_attachment_point, world_attachment_point + world_wheel_direction * suspension_settings->get_rest(), 0.1f, Color(1.0, 1.0, 0.0), false);
		}

		const Vector3 spring_force = world_wheel_direction * (wheel_data.spring_force);
		DebugOverlay::sphere(world_attachment_point + world_wheel_direction * suspension_settings->get_rest(), 0.1f, wheel_data.hit ? Color(1.0f, 1.0f, 0.0f) : Color(1.0f, 0.0f, 0.0f), false);
	}*/
}

void LNVehicle::register_wheel(LNVehicleWheel *p_wheel) {
	ERR_FAIL_COND(p_wheel == nullptr);
	LNVehicleWheelPosition wheel_pos = p_wheel->get_wheel_position();
	ERR_FAIL_INDEX(wheel_pos, WHEEL_MAX);
	ERR_FAIL_COND_MSG(wheels[wheel_pos].wheel != nullptr, "Wheel already exists, bug?");
	wheels[wheel_pos].wheel = p_wheel;
}

void LNVehicle::unregister_wheel(LNVehicleWheel *p_wheel) {
	ERR_FAIL_COND(p_wheel == nullptr);
	LNVehicleWheelPosition wheel_pos = p_wheel->get_wheel_position();
	ERR_FAIL_INDEX(wheel_pos, WHEEL_MAX);
	wheels[wheel_pos].wheel = nullptr;
}

void LNVehicle::set_brake_percentage(float p_brake_percentage) {
	input_state.brake = p_brake_percentage;
}

void LNVehicle::set_steer_percentage(float p_steer_percentage) {
	input_state.steer = p_steer_percentage;
}

void LNVehicle::set_throttle_percentage(float p_throttle_percentage) {
	input_state.throttle = p_throttle_percentage;
}

void LNVehicle::set_clutch_percentage(float p_clutch_precentage) {
	input_state.clutch = p_clutch_precentage;
}

void LNVehicle::request_gear_up() {
	input_state.gear += 1;
}

void LNVehicle::request_gear_down() {
	input_state.gear -= 1;
}

int LNVehicle::get_current_gear() const {
	return gearbox->get_current_gear();
}

float LNVehicle::get_wheel_slip_angle(LNVehicleWheelPosition p_wheel) const {
	ERR_FAIL_INDEX_V(p_wheel, wheels.size(), 0.0f);
	//return wheels[p_wheel].slip_angle;
	return 0.0f;
}

float LNVehicle::get_wheel_slip_ratio(LNVehicleWheelPosition p_wheel) const {
	ERR_FAIL_INDEX_V(p_wheel, wheels.size(), 0.0f);
	//return wheels[p_wheel].slip_ratio;
	return 0.0f;
}

float LNVehicle::get_engine_torque() const {
	return engine->get_output_torque();
}

float LNVehicle::get_engine_rpm() const {
	return engine->get_rpm();
}

void LNVehicle::add_shaft(StringName p_name, Ref<LNVehicleShaft> p_shaft) {
	auto it = shafts.find(p_name);
	ERR_FAIL_COND_MSG(it != shafts.end(), vformat("Tried to add shaft with name %s but shaft with that name already exists!", p_name));

	p_shaft->name = p_name;
	shafts.insert(p_name, p_shaft);
	p_shaft->initialize();
	p_shaft->drivetrain_settings = get_vehicle_settings()->get_drivetrain_settings();
}

void LNVehicle::connect_shaft(StringName p_from, StringName p_to, int p_output) {
	auto from_it = shafts.find(p_from);
	ERR_FAIL_COND_MSG(
			from_it == shafts.end(),
			vformat(
					"Tried to connect shaft %s to %s but parent shaft didn't exist!",
					p_from,
					p_to));
	auto to_it = shafts.find(p_to);
	ERR_FAIL_COND_MSG(
			to_it == shafts.end(),
			vformat(
					"Tried to connect shaft %s to %s but destination shaft didn't exist!",
					p_to,
					p_to));

	ERR_FAIL_INDEX_MSG(
			p_output,
			from_it->value->get_output_count(),
			vformat(
					"Tried to connect shaft %s to %s but the parent shaft only has %d outputs!",
					p_from,
					p_to,
					from_it->value->get_output_count()));

	ERR_FAIL_COND_MSG(
			from_it->value->get_child(p_output) != nullptr,
			vformat(
					"Tried to connect shaft %s to %s but the parent shaft already has a child connected to output %d!",
					p_from,
					p_to,
					p_output));

	ERR_FAIL_COND_MSG(
			from_it->value->get_child(p_output) != nullptr,
			vformat(
					"Tried to connect shaft %s to %s but %s already has a parent!",
					p_from,
					p_to,
					p_to));

	from_it->value->children[p_output] = to_it->value.ptr();
	to_it->value->parent = from_it->value.ptr();
}

void LNVehicle::_ready() {
	/*float longitudinal_stiffness = 10000.0f;  // Cs — N per unit slip ratio
	float cornering_stiffness    = 10000.0f;  // Ca — N per radian of slip angle
	float friction_coefficient   = 0.85f;      // μ  — peak friction, scales with load

	const float vertical_force = (get_mass() / 4.0f) * 9.81f;

	float slip_angle_max = Math::deg_to_rad(15.0f);

	static constexpr int count = 128;

	Ref<FileAccess> fa = FileAccess::open("user://dump.json", FileAccess::WRITE);

	fa->store_line("DATA = [");
	for (int i = -(count/2); i <= ((count / 2)); i++) {
		float angle = (i / static_cast<float>(count/2.0f)) * slip_angle_max;
		Vector2 tire_forces = brush_gdsim(Vector2(angle, 0.0f), 0.35f, friction_coefficient, 0.5f, vertical_force);

		float normalized_force = tire_forces.x / vertical_force;
		fa->store_line(vformat("\t(%.4f, %.4f),", Math::rad_to_deg(angle), normalized_force));
	}
	fa->store_line("]");*/
}

LNVehicle::LNVehicle() {
	set_linear_damp_mode(DAMP_MODE_REPLACE);
	set_linear_damp(0.0f);
}

void LNVehicle::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			// Setup drivetrain stuff

			debugger_window = memnew(Window);
			debugger_window->set_size(get_window()->get_size());
			debugger_window->set_force_native(true);
			add_child(debugger_window);
			debugger = memnew(LNVehicleDrivetrainDebugger);
			debugger_window->add_child(debugger);
			debugger->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);

			const StringName GEARBOX_NAME = StringName("Gearbox");
			const StringName DIFF_NAME = StringName("Differential");
			const StringName CLUTCH_NAME = StringName("Clutch");
			const StringName ENGINE_NAME = StringName("Engine");

			std::array<StringName, 4> WHEEL_NAMES = {
				"Wheel FL",
				"Wheel FR",
				"Wheel RL",
				"Wheel RR"
			};

			static_assert(std::size(WHEEL_NAMES) == WHEEL_MAX);

			engine.instantiate();
			add_shaft(ENGINE_NAME, engine);
			engine->set_engine_settings(get_vehicle_settings()->get_engine_settings());
			engine->set_audio_stream_player(audio_stream_player);
			engine->set_rpm(1000.0f);

			gearbox.instantiate();
			add_shaft(GEARBOX_NAME, gearbox);

			differential.instantiate();
			add_shaft(DIFF_NAME, differential);

			clutch.instantiate();
			add_shaft(CLUTCH_NAME, clutch);

			connect_shaft(CLUTCH_NAME, ENGINE_NAME, 0);
			connect_shaft(CLUTCH_NAME, GEARBOX_NAME, 1);
			connect_shaft(GEARBOX_NAME, DIFF_NAME, 0);

			for (int i = 0; i < wheels.size(); i++) {
				wheels[i].shaft.instantiate();
				if (wheels[i].wheel == nullptr) {
					continue;
				}

				add_shaft(WHEEL_NAMES[wheels[i].wheel->get_wheel_position()], wheels[i].shaft);
			}

			// Now connect the wheels

			connect_shaft(DIFF_NAME, WHEEL_NAMES[WHEEL_RL], 0);
			connect_shaft(DIFF_NAME, WHEEL_NAMES[WHEEL_RR], 1);

			callable_mp(debugger, &LNVehicleDrivetrainDebugger::update_tree).call_deferred(this);
		};
	}
}
