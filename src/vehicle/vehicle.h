#pragma once

#include "../console/cvar.h"
#include "bind_macros.h"
#include "godot_cpp/classes/audio_stream_player.hpp"
#include "godot_cpp/classes/rigid_body3d.hpp"
#include "godot_cpp/classes/window.hpp"
#include "godot_cpp/core/binder_common.hpp"
#include "vehicle/clutch.h"
#include "vehicle/shaft.h"
#include "vehicle/telemetry/vehicle_telemetry.h"
#include "vehicle/telemetry/vehicle_telemetry_window.h"
#include "vehicle/vehicle_differential.h"
#include "vehicle/vehicle_drivetrain_debugger.h"
#include "vehicle/vehicle_engine.h"
#include "vehicle/vehicle_gearbox.h"
#include "vehicle/vehicle_settings.h"
#include "vehicle/vehicle_wheel_shaft.h"
#include "wheel_position.h"

#include <optional>

using namespace godot;

class LNVehicleWheel;

class LNVehicle : public RigidBody3D {
	GDCLASS(LNVehicle, RigidBody3D);

public:
	struct WheelData {
		LNVehicleWheel *wheel = nullptr;
		Ref<LNVehicleWheelShaft> shaft;
	};

	Window *debugger_window = nullptr;

#ifdef DEBUG_ENABLED
	VehicleTelemetry telemetry;
	VehicleTelemetryControl *telemetry_control = nullptr;
#endif
	LNVehicleDrivetrainDebugger *drivetrain_debugger = nullptr;

	VehicleInputState input_state;
	void _apply_arb(int p_wheel_left, int p_wheel_right, float p_arb_stiffness);
	AudioStreamPlayer *audio_stream_player = nullptr;

	MAKE_SETTER_GETTER_VALUE(AudioStreamPlayer *, audio_stream_player, audio_stream_player)

	std::array<WheelData, static_cast<size_t>(LNVehicleWheelPosition::WHEEL_MAX)> wheels = {};

	CVar *vehicle_draw_wheels_cvar;

	Ref<LNVehicleSettings> vehicle_settings;

	// Nodes
	Ref<LNVehicleEngine> engine;
	Ref<LNVehicleClutchNode> clutch;
	Ref<LNVehicleDifferential> differential;
	Ref<LNVehicleGearbox> gearbox;

	HashMap<StringName, Ref<LNVehicleShaft>> shafts;

	MAKE_SETTER_GETTER_VALUE(Ref<LNVehicleSettings>, vehicle_settings, vehicle_settings);

	void _apply_force(Vector3 p_force_global, Vector3 p_offset_global, std::optional<Color> p_color = {});

	static void _bind_methods();
	virtual void _physics_process(double p_delta) override;
	void _debug_draw();

	void register_wheel(LNVehicleWheel *p_wheel);
	void unregister_wheel(LNVehicleWheel *p_wheel);
	void _initialize();
	void set_brake_percentage(float p_brake_percentage);
	void set_steer_percentage(float p_steer_percentage);
	void set_throttle_percentage(float p_throttle_percentage);
	void set_clutch_percentage(float p_clutch_precentage);
	void request_gear_up();
	void request_gear_down();
	int get_current_gear() const;
	float get_wheel_slip_angle(LNVehicleWheelPosition p_wheel) const;
	float get_wheel_slip_ratio(LNVehicleWheelPosition p_wheel) const;
	float get_wheel_angular_velocity(LNVehicleWheelPosition p_wheel) const;
	float get_engine_torque() const;
	float get_engine_rpm() const;

	void add_shaft(StringName p_name, Ref<LNVehicleShaft> p_shaft);
	void connect_shaft(StringName p_from, StringName p_to, int p_output);
	virtual void _ready() override;
	LNVehicle();
	void _notification(int p_what);

	static String wheel_position_to_string(LNVehicleWheelPosition p_wheel_pos) {
		switch (p_wheel_pos) {
			case WHEEL_FL: {
				return "fl";
			} break;
			case WHEEL_FR: {
				return "fr";
			} break;
			case WHEEL_RL: {
				return "rl";
			} break;
			case WHEEL_RR: {
				return "rr";
			} break;
			case WHEEL_MAX: {
				DEV_ASSERT(false);
				return "";
			} break;
		}
	}

	friend class LNVehicleDrivetrainDebugger;
};

VARIANT_ENUM_CAST(LNVehicleWheelPosition);
