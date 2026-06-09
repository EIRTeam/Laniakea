#pragma once

#include "debug/debug_overlay.h"
#include "godot_cpp/classes/geometry3d.hpp"
#include "godot_cpp/classes/input.hpp"
#include "godot_cpp/classes/node3d.hpp"
#include "godot_cpp/classes/physics_direct_space_state3d.hpp"
#include "godot_cpp/classes/physics_ray_query_parameters3d.hpp"
#include "godot_cpp/classes/world3d.hpp"
#include "godot_cpp/core/math.hpp"
#include "godot_cpp/variant/plane.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "math.h"
#include "vehicle/vehicle_suspension.h"
#include "vehicle/vehicle_suspension_macpherson_settings.h"
#include "vehicle/vehicle_suspension_settings.h"
#include "vehicle/vehicle_suspension_state.h"
#include "vehicle/vehicle_wheel_settings.h"
#include "vehicle/wheel_position.h"

using namespace godot;
class SuspensionTest : public Node3D {
    GDCLASS(SuspensionTest, Node3D);
public:

    LNVehicleSuspensionState* state_left;
    LNVehicleSuspensionState* state_right;
    Ref<LNVehicleMacPhersonSuspensionSettings> settings;
    Ref<LNVehicleWheelSettings> wheel_settings;
    float steer = 0.0f;

    SuspensionTest() {
        settings.instantiate();
        settings->set_bottom_wishbone_front(Vector3(0.5, -0.25, -0.324));
        settings->set_bottom_wishbone_rear(Vector3(0.5, -0.25, 0.041));
        settings->set_bottom_wishbone_tyre(Vector3(0.053, -0.25, -0.016));
        settings->set_strut_car(Vector3(0.281, 0.1, 0.026));
        settings->set_static_camber_degrees(-0.0f);
        state_left = settings->create_suspension_state();
        state_right = settings->create_suspension_state();
        wheel_settings.instantiate();
        wheel_settings->set_radius(0.3f);
    }

    virtual void _physics_process(double p_delta) override {

        const float steering_target = Input::get_singleton()->get_action_strength("steer_right") - Input::get_singleton()->get_action_strength("steer_left");

        steer = Math::move_toward(steer, steering_target, (float)Math_PI * (float)p_delta);

        state_left->steering_angle_rads = steer;
        state_right->steering_angle_rads = steer;

        set_global_basis(Basis().rotated(Vector3(0.0, 1.0, 0.0), Math::deg_to_rad(0.0f)));
        Transform3D trf_left;
        trf_left.origin.x = -0.6f;
        Transform3D trf_right;
        trf_right.origin.x = 0.6f;
        trf_right.basis.scale_local(Vector3(-1.0f, 1.0f, 1.0f));
        _solve_macpherson(trf_left, WHEEL_FL, p_delta, reinterpret_cast<LNVehicleMacPhersonSuspensionSettings::LNVehicleMacPhersonSuspensionState*>(state_left));
        _solve_macpherson(trf_right, WHEEL_FR, p_delta, reinterpret_cast<LNVehicleMacPhersonSuspensionSettings::LNVehicleMacPhersonSuspensionState*>(state_right));
    }
    virtual void _solve_macpherson(const Transform3D &p_suspension_transform, const LNVehicleWheelPosition p_wheel_pos, double p_delta, LNVehicleMacPhersonSuspensionSettings::LNVehicleMacPhersonSuspensionState *p_state) {
        p_state->suspension_transform_world = p_suspension_transform * get_global_transform();
        settings->solve(wheel_settings, p_wheel_pos, this, p_delta, p_state);
    }

    static void _bind_methods() {}
};