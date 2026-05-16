#include "vehicle.h"
#include "bind_macros.h"
#include "debug/debug_overlay.h"
#include "godot_cpp/classes/physics_direct_space_state3d.hpp"
#include "godot_cpp/classes/physics_ray_query_parameters3d.hpp"
#include "godot_cpp/classes/rigid_body3d.hpp"
#include "godot_cpp/classes/world3d.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/math_defs.hpp"
#include "vehicle_suspension_settings.h"
#include "vehicle_wheel.h"
#include "vehicle_wheel_settings.h"
#include "../physics.h"

void LNVehicle::_apply_force(Vector3 p_force_global, Vector3 p_offset_global) {
    DEV_ASSERT(p_force_global.is_finite());
    if (const Vector3 force_visual = p_force_global / 500.0f; !force_visual.is_zero_approx()) {
        DebugOverlay::filled_arrow(get_global_position()+p_offset_global, get_global_position()+p_offset_global+force_visual, 0.15f, Color(0.0, 1.0, 1.0), false);
    }
    apply_force(p_force_global, p_offset_global);
}

void LNVehicle::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_brake_percentage", "brake"), &LNVehicle::set_brake_percentage);
    ClassDB::bind_method(D_METHOD("set_steer_percentage", "steer"), &LNVehicle::set_steer_percentage);
    MAKE_BIND_NODE(LNVehicle, audio_stream_player, AudioStreamPlayer);
}

Vector2 calculate_transient_slip(LNVehicle::WheelData &p_wheel, Vector2 p_wheel_velocity, double p_delta_inv) {
    static constexpr float longitudinal_relaxation_length = 0.1f;
    static constexpr float lateral_relaxation_length = 0.3f;    

    const Vector2 contact_patch_velocity = p_wheel_velocity;
    const float wheel_radius = p_wheel.wheel->get_wheel_settings()->get_radius();

    Vector2 slip_velocity = Vector2(contact_patch_velocity.x, p_wheel.angular_velocity * wheel_radius - contact_patch_velocity.y);

    const float velocity_forward_abs = MAX(Math::abs(contact_patch_velocity.y), 0.5f);
    const float steady_state_slip_ratio = slip_velocity.y / velocity_forward_abs;
    const float slip_ratio_delta = (slip_velocity.y - velocity_forward_abs * p_wheel.slip_ratio) / longitudinal_relaxation_length;
    float new_slip_ratio = p_wheel.slip_ratio + slip_ratio_delta * p_delta_inv;
    new_slip_ratio = CLAMP(new_slip_ratio, -Math::abs(steady_state_slip_ratio), Math::abs(steady_state_slip_ratio));
    
    const float steady_state_slip_angle = Math::atan2(contact_patch_velocity.x, velocity_forward_abs);
    const float tan_slip_angle_delta = (steady_state_slip_angle - velocity_forward_abs * p_wheel.slip_angle) / lateral_relaxation_length;
    const float min_a = Math::abs(Math::tan(steady_state_slip_angle));
    const float new_slip_angle = CLAMP(p_wheel.slip_angle + tan_slip_angle_delta * p_delta_inv, -min_a, min_a);

    return Vector2(new_slip_angle, new_slip_ratio);
}

Vector2 brush(Vector2 slip, Vector2 p_stiffness, float friction, float load) {
    // 1. Calculate combined deflection magnitude
    // slip.x = lateral slip (tan of slip angle), slip.y = longitudinal slip ratio
    float deflection = Math::sqrt(Math::pow(p_stiffness.y * slip.y, 2) + 
                                Math::pow(p_stiffness.x * slip.x, 2));

    if (deflection == 0.0f) return {0.0f, 0.0f};

    // 2. Critical limit (where sliding begins)
    // contact_patch_length is often normalized to 1.0 for simplicity
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
        force.y = friction_force * (p_stiffness.x * slip.y) * brush_factor;
    }

    return force;
}

void LNVehicle::_physics_process(double p_delta) {
    if (Engine::get_singleton()->is_editor_hint()) {
        _debug_draw();
        return;
    }
    Ref<PhysicsRayQueryParameters3D> raycast_params;
    raycast_params.instantiate();
    raycast_params->set_exclude({this});

    PhysicsDirectSpaceState3D *dss = get_world_3d()->get_direct_space_state();
    for (int wheel_idx = 0; wheel_idx < WHEEL_MAX; wheel_idx++) {
        WheelData &wheel_data = wheels[wheel_idx];
        LNVehicleWheel *wheel = wheels[wheel_idx].wheel;

        if (wheel == nullptr) {
            continue;
        }

        wheel_data.tire_force_lateral = 0.0f;
        wheel_data.tire_force_longitudinal = 0.0f;

        Ref<LNVehicleSuspensionSettings> suspension_settings = wheel->get_suspension_settings();
        Ref<LNVehicleWheelSettings> wheel_settings = wheel->get_wheel_settings();
        if (suspension_settings.is_null() || wheel_settings.is_null()) {
            continue;
        }

        const Vector3 world_attachment_point = to_global(wheel->get_top_attachment_point());
        raycast_params->set_from(world_attachment_point);
        // Straight down, for now
        const Vector3 world_wheel_direction = get_global_basis().xform(Vector3(0.0, -1.0, 0.0));
        const float wheel_radius = wheel_settings->get_radius();
        raycast_params->set_from(world_attachment_point);
        raycast_params->set_to(world_attachment_point + world_wheel_direction * (suspension_settings->get_maximum() + wheel_settings->get_radius()));

        Dictionary result = dss->intersect_ray(raycast_params);
        wheel_data.hit = !result.is_empty();
        if (!wheel_data.hit) {
            // Freewheel
            _process_wheel_airborne(wheel_data, world_wheel_direction, world_attachment_point, p_delta);
        } else {
            const float new_extension = world_attachment_point.distance_to(result["position"]) - wheel_settings->get_radius();
            wheel_data.hit_position = result["position"];
            wheel_data.spring_displacement = new_extension - suspension_settings->get_rest();
            _process_wheel_grounded(wheel_data, world_wheel_direction, p_delta);
        }

        Vector3 wheel_pos = world_attachment_point + world_wheel_direction * (suspension_settings->get_rest() + wheel_data.spring_displacement);
        wheel->set_global_position(wheel_pos);

        const float wheel_mass = wheel_settings->get_mass();
        const float wheel_moment = 0.5f * wheel_mass * wheel_radius * wheel_radius;
        const float max_brake_torque = Math::abs(wheel_data.drive_torque) + ((Math::abs(wheel_data.angular_velocity) * wheel_moment) / p_delta);

        wheel_data.brake_torque = SIGN(wheel_data.angular_velocity) * MIN(max_brake_torque, input.brake_percentage * 1000.0f);

        const float T_ext = wheel_data.drive_torque - wheel_data.brake_torque;

        DEV_ASSERT(Math::is_finite(T_ext));
        DEV_ASSERT(Math::is_finite(wheel_radius));

        // Integrate wheel using the clamped force
        wheel_data.angular_velocity += ((T_ext - wheel_data.tire_force_longitudinal * wheel_radius)
                                    / wheel_moment) * p_delta;
        wheel_data.angular_velocity *= (1.0f - 0.01f * p_delta);

        wheel->rotate_object_local(Vector3(1.0f, 0.0f, 0.0f), -wheel_data.angular_velocity * p_delta);
    }

    _debug_draw();
}

void LNVehicle::_process_wheel_grounded(WheelData &p_wheel, const Vector3 &p_world_wheel_direction, float p_delta)
{
    const Ref<LNVehicleSuspensionSettings> suspension_settings = p_wheel.wheel->get_suspension_settings();
    const float spring_rate = suspension_settings->get_spring_rate(get_mass() / 4.0f);
    const float damping_coeff = suspension_settings->get_damping_coefficient(get_mass() / 4.0f);
    
    const Vector3 contact_velocity = LNPhysics::velocity_at_pos(this, p_wheel.hit_position);
    const float contact_velocity_along_spring = p_world_wheel_direction.dot(contact_velocity);

    const float spring_force =  spring_rate * p_wheel.spring_displacement;
    const float damping_force = damping_coeff * contact_velocity_along_spring;
    const float out_spring_force = spring_force - damping_force;

    p_wheel.spring_force = MIN(out_spring_force, 0.0f);

    const Ref<LNVehicleWheelSettings> wheel_settings = p_wheel.wheel->get_wheel_settings();

    const Vector3 world_wheel_forward = wheel_get_world_forward(p_wheel.wheel);
    const Vector3 world_wheel_right = wheel_get_world_right(p_wheel.wheel);

    const float longitudinal_wheel_speed = contact_velocity.dot(world_wheel_forward);
    const float lateral_wheel_speed = contact_velocity.dot(world_wheel_right);

    _apply_force(p_world_wheel_direction * (p_wheel.spring_force), p_wheel.hit_position - get_global_position());

    // Time to simulate tires
    const Vector2 transient_slip = calculate_transient_slip(p_wheel, Vector2(-lateral_wheel_speed, longitudinal_wheel_speed), p_delta);
        
    p_wheel.slip_ratio = transient_slip.y;
    p_wheel.slip_angle = transient_slip.x;
        
    float longitudinal_stiffness = 80000.0f;  // Cs — N per unit slip ratio
    float cornering_stiffness    = 60000.0f;  // Ca — N per radian of slip angle  
    float friction_coefficient   = 1.0f;      // μ  — peak friction, scales with load

    Vector2 tire_forces = brush(Vector2(p_wheel.slip_angle, p_wheel.slip_ratio), Vector2(cornering_stiffness, longitudinal_stiffness), friction_coefficient, -p_wheel.spring_force);
    p_wheel.tire_force_longitudinal = tire_forces.y;
    p_wheel.tire_force_lateral = tire_forces.x;

    _apply_force(world_wheel_forward * (p_wheel.tire_force_longitudinal),
            p_wheel.hit_position - get_global_position());
    _apply_force(world_wheel_right * p_wheel.tire_force_lateral,
            p_wheel.hit_position - get_global_position());
}

void LNVehicle::_process_wheel_airborne(WheelData &p_wheel, const Vector3 &p_world_wheel_direction, const Vector3 &p_world_attachment_point, float p_delta) {
    const Ref<LNVehicleSuspensionSettings> suspension_settings = p_wheel.wheel->get_suspension_settings();
    const float spring_rate = suspension_settings->get_spring_rate(get_mass() / 4.0f);
    const float damping_coeff = suspension_settings->get_damping_coefficient(get_mass() / 4.0f);
    const float g_along_spring = get_gravity().dot(p_world_wheel_direction); // project gravity onto spring axis

    const float unsprung_mass = p_wheel.wheel->get_wheel_settings()->get_mass();

    // Force balance on unsprung mass: gravity tries to extend, spring+damper resist
    const float f_net = unsprung_mass * g_along_spring
                      - spring_rate * p_wheel.spring_displacement
                      - damping_coeff * p_wheel.spring_velocity;

    p_wheel.spring_velocity += (f_net / unsprung_mass) * p_delta;
    p_wheel.spring_displacement += p_wheel.spring_velocity * p_delta;

    // Clamp to suspension travel limits
    p_wheel.spring_displacement = CLAMP(p_wheel.spring_displacement, suspension_settings->get_minimum(), suspension_settings->get_maximum());
}

void LNVehicle::_debug_draw() {
    for (int wheel_idx = 0; wheel_idx < WHEEL_MAX; wheel_idx++) {
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
        DebugOverlay::sphere(world_attachment_point + world_wheel_direction * suspension_settings->get_rest(), 0.1f, Color(1.0f, 0.0f, 0.0f), false);
    }
}

Vector3 LNVehicle::wheel_get_world_forward(LNVehicleWheel *p_wheel) const {
    if (p_wheel->get_steerable()) {
        return get_global_basis().xform(Vector3(0.0f, 0.0f, -1.0f).rotated(Vector3(0.0, 1.0, 0.0), input.steer * Math_PI * -0.25f));
    }
    return get_global_basis().xform(Vector3(0.0f, 0.0f, -1.0f));
}

Vector3 LNVehicle::wheel_get_world_right(LNVehicleWheel *p_wheel) const {
    if (p_wheel->get_steerable()) {
        return get_global_basis().xform(Vector3(1.0f, 0.0f, 0.0f).rotated(Vector3(0.0, 1.0, 0.0), input.steer * Math_PI * -0.25f));
    }
    return get_global_basis().xform(Vector3(1.0f, 0.0f, 0.0f));
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
    input.brake_percentage = p_brake_percentage;
}

void LNVehicle::set_steer_percentage(float p_steer_percentage) {
    input.steer = p_steer_percentage;
}

LNVehicle::LNVehicle() {
    set_linear_damp_mode(DAMP_MODE_REPLACE);
    set_linear_damp(0.0f);
}
