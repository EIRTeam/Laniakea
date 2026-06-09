#pragma once

#include "../bind_macros.h"
#include "godot/property_list_helper.h"
#include "godot_cpp/classes/resource.hpp"
#include "vehicle_suspension_state.h"
#include "wheel_position.h"

using namespace godot;

class LNVehicleWheelSettings;

namespace godot {
class Node3D;
}

class LNVehicleSuspensionSettings : public Resource {
    GDCLASS(LNVehicleSuspensionSettings, Resource);
    // 0 wheel height relative point at which bump rate starts being applied
    float bumpstop_up = 0.080f;
    float bumpstop_down = 0.080f;

    // Compression length at which the packer rate will start to be applied
    float packer_range = 0.0f;
    
    // Offset from design 0 where the suspension is resting.
    float rod_length_offset = 0.0f;

    float bumpstop_rate = 30000.0f;

    float spring_rate = 45760.0f;
    
    float bump_damp_rate = 3273.0f;
    float bump_fast_damp_rate = 1934.0f;
    float bump_fast_damp_threshold = 0.080f;

    float rebound_damp_rate = 5875.0f;
    float rebound_fast_damp_rate = 2601.0f;
    float rebound_fast_damp_rate_threshold = 0.130f;

    float toe_out = -0.00030f;

    // In degrees
    float static_camber_degrees = -1.6f;
protected:
    struct WishboneSettings {
        Vector3 front;
        Vector3 rear;
        Vector3 tyre;
    };
    struct GeometrySettings {
        Vector3 strut_car;
        Vector3 strut_tyre;
    } geometry;
public:
    MAKE_SETTER_GETTER_FLOAT_VALUE(bumpstop_up, bumpstop_up);
    MAKE_SETTER_GETTER_FLOAT_VALUE(bumpstop_down, bumpstop_down);
    MAKE_SETTER_GETTER_FLOAT_VALUE(packer_range, packer_range);
    MAKE_SETTER_GETTER_FLOAT_VALUE(rod_length_offset, rod_length_offset);
    MAKE_SETTER_GETTER_FLOAT_VALUE(bumpstop_rate, bumpstop_rate);
    MAKE_SETTER_GETTER_FLOAT_VALUE(spring_rate, spring_rate);

    MAKE_SETTER_GETTER_FLOAT_VALUE(bump_damp_rate, bump_damp_rate);
    MAKE_SETTER_GETTER_FLOAT_VALUE(bump_fast_damp_rate, bump_fast_damp_rate);
    MAKE_SETTER_GETTER_FLOAT_VALUE(bump_fast_damp_threshold, bump_fast_damp_threshold);

    MAKE_SETTER_GETTER_FLOAT_VALUE(rebound_damp_rate, rebound_damp_rate);
    MAKE_SETTER_GETTER_FLOAT_VALUE(rebound_fast_damp_rate, rebound_fast_damp_rate);
    MAKE_SETTER_GETTER_FLOAT_VALUE(rebound_fast_damp_rate_threshold, rebound_fast_damp_rate_threshold);


    MAKE_SETTER_GETTER_FLOAT_VALUE(toe_out, toe_out);
    MAKE_SETTER_GETTER_FLOAT_VALUE(static_camber_degrees, static_camber_degrees);

    MAKE_SETTER_GETTER_VALUE(Vector3, strut_car, geometry.strut_car);

    static void _bind_methods();

    virtual LNVehicleSuspensionState* create_suspension_state() const = 0;

    struct SuspensionSolveResult {
        bool success = false;
        bool grounded = false;
        Vector3 force_to_apply;
        Vector3 force_world_position;
        Transform3D wheel_transform;
        Vector3 grounded_normal;
        Vector3 ground_hit_position;
    };
    virtual SuspensionSolveResult solve(Ref<LNVehicleWheelSettings> p_wheel_settings, LNVehicleWheelPosition p_wheel_position, const Node3D *p_vehicle, double p_delta, LNVehicleSuspensionState *p_state) = 0;
};