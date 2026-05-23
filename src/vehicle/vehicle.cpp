#include "vehicle.h"
#include "bind_macros.h"
#include "debug/debug_overlay.h"
#include "godot_cpp/classes/file_access.hpp"
#include "godot_cpp/classes/physics_direct_space_state3d.hpp"
#include "godot_cpp/classes/physics_ray_query_parameters3d.hpp"
#include "godot_cpp/classes/rigid_body3d.hpp"
#include "godot_cpp/classes/world3d.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/math_defs.hpp"
#include "math.h"
#include "vehicle_suspension_settings.h"
#include "vehicle_wheel.h"
#include "vehicle_wheel_settings.h"
#include "../physics.h"
#include <cfenv>

void LNVehicle::_apply_arb(int p_wheel_left, int p_wheel_right, float p_arb_stiffness) {
    WheelData &left  = wheels[p_wheel_left];
    WheelData &right = wheels[p_wheel_right];

    if (!left.hit || !right.hit) return;

    const float arb_torque = p_arb_stiffness
                           * (left.spring_displacement - right.spring_displacement);

    // Use contact normals, consistent with spring force
    _apply_force(-left.contact_normal  * arb_torque,
            left.hit_position  - get_global_position(), Color(1.0, 0.0, 0.0));
    _apply_force( right.contact_normal * arb_torque,
            right.hit_position - get_global_position(), Color(1.0, 0.0, 0.0));
}

void LNVehicle::_apply_force(Vector3 p_force_global, Vector3 p_offset_global, std::optional<Color> p_color) {
    DEV_ASSERT(p_force_global.is_finite());
    if (const Vector3 force_visual = p_force_global / 500.0f; !force_visual.is_zero_approx()) {
        Color color = p_color.value_or(Color(0.0, 1.0, 1.0));
        DebugOverlay::filled_arrow(get_global_position()+p_offset_global, get_global_position()+p_offset_global+force_visual, 0.15f, color, false);
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

void calculate_transient_slip(LNVehicle::WheelData &p_wheel, Vector2 p_wheel_velocity, double p_delta) {
    static constexpr float longitudinal_relaxation_length = 0.1f;
    static constexpr float lateral_relaxation_length = 0.3f;    
    const float clamping_factor = 1.0f;

    const Vector2 contact_patch_velocity = p_wheel_velocity;
    const float wheel_radius = p_wheel.wheel->get_wheel_settings()->get_radius();
    Vector2 slip_velocity = Vector2(contact_patch_velocity.x, p_wheel.angular_velocity * wheel_radius - contact_patch_velocity.y);
    const float steady_state_slip_angle = Math::atan(slip_velocity.x / MAX(Math::abs(contact_patch_velocity.y), 1.0f));
    const float slip_angle_delta = (p_delta / lateral_relaxation_length) * (slip_velocity.x - MAX(contact_patch_velocity.y, clamping_factor) * p_wheel.slip_angle);
    p_wheel.differential_tan_slip_angle = CLAMP(p_wheel.differential_tan_slip_angle + slip_angle_delta, -Math::abs(Math::tan(steady_state_slip_angle)), Math::abs(Math::tan(steady_state_slip_angle)));
    
    const float steady_state_slip_ratio = slip_velocity.y / MAX(Math::abs(contact_patch_velocity.y), clamping_factor);
    float slip_ratio_delta = (p_delta / longitudinal_relaxation_length) * (slip_velocity.y - MAX(Math::abs(contact_patch_velocity.y), clamping_factor) * p_wheel.slip_ratio);
    p_wheel.slip_ratio = CLAMP(p_wheel.slip_ratio + slip_ratio_delta, -Math::abs(steady_state_slip_ratio), Math::abs(steady_state_slip_ratio));
    p_wheel.slip_angle = Math::atan(p_wheel.differential_tan_slip_angle);
}

Vector2 brush_gdsim(Vector2 p_slip, float p_contact_patch, float p_coefficient_of_friction, float p_tire_stiffness, float p_y_force) {
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
}

Vector2 brush(Vector2 slip, Vector2 p_stiffness, float friction, float load) {
    float deflection = Math::sqrt(Math::pow(p_stiffness.y * slip.y, 2) + 
                                Math::pow(p_stiffness.x * slip.x, 2));

    if (deflection == 0.0f) return {0.0f, 0.0f};

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

    while (input.gear > 0) {
        input.gear--;
        drivetrain->gear_up();
    }
    while (input.gear < 0) {
        input.gear++;
        drivetrain->gear_down();
    }

    engine->set_engine_settings(vehicle_settings->get_engine_settings());
    drivetrain->set_drivetrain_settings(vehicle_settings->get_drivetrain_settings());
    Ref<PhysicsRayQueryParameters3D> raycast_params;
    raycast_params.instantiate();
    raycast_params->set_exclude({this});

    PhysicsDirectSpaceState3D *dss = get_world_3d()->get_direct_space_state();

    // Autoclutch: disengage below idle RPM, blend in above it. Take the more-disengaged of the two inputs.
    const float autoclutch_min = vehicle_settings->get_drivetrain_settings()->get_autoclutch_min();
    const float autoclutch_max = vehicle_settings->get_drivetrain_settings()->get_autoclutch_max();
    const float clutch_amount = 1.0f - CLAMP(
        Math::inverse_lerp(autoclutch_min, autoclutch_max, engine->get_rpm()),
        0.0f, 1.0f
    );

    const float rd = wheels[WHEEL_RL].wheel->get_wheel_settings()->get_radius();
    const float inert = 0.5f * wheels[WHEEL_RL].wheel->get_wheel_settings()->get_mass() * rd * rd;
    Vector2 rear_wheel_angular_velocities = Vector2(wheels[WHEEL_RL].angular_velocity, wheels[WHEEL_RR].angular_velocity);
    drivetrain->update(engine, input.throttle, MAX(clutch_amount, input.clutch), wheels[WHEEL_RL].angular_velocity, wheels[WHEEL_RR].angular_velocity, inert, p_delta);

    const float wheel_masss = wheels[WHEEL_RL].wheel->get_wheel_settings()->get_mass();
    const float wheel_radiuss = wheels[WHEEL_RL].wheel->get_wheel_settings()->get_radius();

    float gearbox_downstream_inertia;
    drivetrain->gearbox_get_downstream_inertia(gearbox_downstream_inertia, 0.0f);
    float diff_inertia_l;
    float diff_inertia_r;
    drivetrain->diff_get_downstream_inertia(diff_inertia_l, diff_inertia_r, gearbox_downstream_inertia);
    
    float downstream_torque = drivetrain->gearbox_get_downstream_torque();

    float wheel_moments[4];

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
            wheel_data.contact_normal = result["normal"];
            Vector3 to_contact = Vector3(result["position"]) - world_attachment_point;
            float new_extension = to_contact.dot(world_wheel_direction)
                                - wheel_settings->get_radius();
            wheel_data.hit_position = result["position"];
            wheel_data.spring_displacement = new_extension - suspension_settings->get_rest();
            _process_wheel_grounded(wheel_data, world_wheel_direction, p_delta);
        }
        const float wheel_mass = wheel_settings->get_mass();
        wheel_moments[wheel_idx] = (0.5f * wheel_mass * wheel_radius * wheel_radius);
        if (wheel_idx == WHEEL_RL || wheel_idx == WHEEL_RR) {
            wheel_moments[wheel_idx] += wheel_idx == WHEEL_RL ? diff_inertia_l : diff_inertia_r;
        }
    }

    for (int wheel_idx = 0; wheel_idx < WHEEL_MAX; wheel_idx++) {

        WheelData &wheel_data = wheels[wheel_idx];
        LNVehicleWheel *wheel = wheels[wheel_idx].wheel;
        Ref<LNVehicleWheelSettings> wheel_settings = wheel->get_wheel_settings();
        const Vector3 world_attachment_point = to_global(wheel->get_top_attachment_point());
        const float wheel_radius = wheel_settings->get_radius();
        const Vector3 world_wheel_direction = get_global_basis().xform(Vector3(0.0, -1.0, 0.0));
        Ref<LNVehicleSuspensionSettings> suspension_settings = wheel->get_suspension_settings();
        Vector3 wheel_pos = world_attachment_point + world_wheel_direction * (suspension_settings->get_rest() + wheel_data.spring_displacement);
        wheel->set_global_position(wheel_pos);

        const float wheel_mass = wheel_settings->get_mass();
        if (wheel_idx == WHEEL_RL || wheel_idx == WHEEL_RR) {
            const Vector2 wheel_torques = Vector2(wheels[WHEEL_RL].tire_force_longitudinal * wheels[WHEEL_RL].wheel->get_wheel_settings()->get_radius(), wheels[WHEEL_RR].tire_force_longitudinal * wheels[WHEEL_RR].wheel->get_wheel_settings()->get_radius());
            Vector2 diff_out = drivetrain->GetDownstreamTorque(downstream_torque, rear_wheel_angular_velocities, wheel_torques, Vector2(wheel_moments[WHEEL_RL], wheel_moments[WHEEL_RR]), p_delta);
            wheel_data.drive_torque = (wheel_idx == WHEEL_RL) ? diff_out.x : diff_out.y;
            wheel_data.drive_torque -= wheels[wheel_idx].tire_force_longitudinal * wheel_radius;
        }

        const float T_ext = wheel_data.drive_torque + wheels[wheel_idx].tire_force_longitudinal * wheel_radius;

        DEV_ASSERT(Math::is_finite(T_ext));
        DEV_ASSERT(Math::is_finite(wheel_radius));
        
        const float wheel_moment = wheel_moments[wheel_idx];
        const float torque_required_to_stop_wheel = -(T_ext + ((wheel_data.angular_velocity*wheel_moment)/p_delta));
        const float brake_torque_available = 1000.0f * input.brake_percentage;
        wheel_data.brake_torque = CLAMP(torque_required_to_stop_wheel, -brake_torque_available, brake_torque_available);

        // Integrate wheel using the clamped force
        wheel_data.angular_velocity += ((T_ext + wheel_data.brake_torque)
                                    / wheel_moment) * p_delta;
        wheel_data.angle += wheel_data.angular_velocity * p_delta;
        // wheel->set_rotation(Vector3(wheel_data.angle * p_delta, wheel->get_steerable() ? input.steer * Math_PI * -0.25f : 0.0f, 0.0f));
        wheel->set_rotation(Vector3(-wheel_data.angle, wheel->get_steerable() ? input.steer * Math_PI * -0.25f : 0.0f, 0.0f));

        DebugOverlay::text(wheel_pos, vformat("%.2f", wheel_data.angular_velocity), Color(0.0, 0.0, 0.0, 1.0f));
    }
    // Front axle ARB
    _apply_arb(WHEEL_FL, WHEEL_FR,
            vehicle_settings->get_front_arb_stiffness());

    // Rear axle ARB
    _apply_arb(WHEEL_RL, WHEEL_RR,
            vehicle_settings->get_rear_arb_stiffness());

    _debug_draw();
}

void LNVehicle::_process_wheel_grounded(WheelData &p_wheel, const Vector3 &p_world_wheel_direction, float p_delta)
{
    const Ref<LNVehicleSuspensionSettings> suspension_settings = p_wheel.wheel->get_suspension_settings();
    const float spring_rate = suspension_settings->get_spring_rate();

    const Vector3 contact_velocity = LNPhysics::velocity_at_pos(this, p_wheel.hit_position);
    const float contact_velocity_along_spring = p_world_wheel_direction.dot(contact_velocity);

    // Positive velocity = compressing (bump), negative = extending (rebound)
    const float critical_damp = Math::sqrt(spring_rate * get_mass() / 4.0f);
    const float damping_coeff = (contact_velocity_along_spring >= 0.0f)
        ? suspension_settings->get_bump() * critical_damp
        : suspension_settings->get_rebound() * critical_damp;

    const float spring_force  = spring_rate  * p_wheel.spring_displacement;
    const float damping_force = damping_coeff * contact_velocity_along_spring;
    const float out_spring_force = spring_force - damping_force;
    p_wheel.spring_force = MIN(out_spring_force, 0.0f);

    const Ref<LNVehicleWheelSettings> wheel_settings = p_wheel.wheel->get_wheel_settings();

    const Vector3 world_wheel_forward = wheel_get_world_forward(p_wheel.wheel);
    const Vector3 world_wheel_right = wheel_get_world_right(p_wheel.wheel);

    DebugOverlay::filled_arrow(p_wheel.wheel->get_global_position(), p_wheel.wheel->get_global_position() + world_wheel_right * 2.0f, 0.1f, Color(0.0, 1.0, 0.0));

    const float longitudinal_wheel_speed = contact_velocity.dot(world_wheel_forward);
    const float lateral_wheel_speed = contact_velocity.dot(world_wheel_right);

    _apply_force( -p_wheel.contact_normal * (p_wheel.spring_force), p_wheel.hit_position - get_global_position());

    calculate_transient_slip(p_wheel, Vector2(lateral_wheel_speed, longitudinal_wheel_speed), p_delta);
        
    Vector2 tire_forces = brush_gdsim(Vector2(p_wheel.slip_angle, p_wheel.slip_ratio), wheel_settings->get_contact_patch(), wheel_settings->get_coefficient_of_friction(), wheel_settings->get_stiffness(), -p_wheel.spring_force);
    p_wheel.tire_force_longitudinal = tire_forces.y;
    p_wheel.tire_force_lateral = tire_forces.x;

    _apply_force(world_wheel_forward * (-p_wheel.tire_force_longitudinal),
            p_wheel.hit_position - get_global_position());
    _apply_force(world_wheel_right * (-p_wheel.tire_force_lateral),
            p_wheel.hit_position - get_global_position());
}

void LNVehicle::_process_wheel_airborne(WheelData &p_wheel, const Vector3 &p_world_wheel_direction, const Vector3 &p_world_attachment_point, float p_delta) {
    const Ref<LNVehicleSuspensionSettings> suspension_settings = p_wheel.wheel->get_suspension_settings();
    const float spring_rate = suspension_settings->get_spring_rate();
    const float g_along_spring = get_gravity().dot(p_world_wheel_direction); // project gravity onto spring axis

    const float unsprung_mass = p_wheel.wheel->get_wheel_settings()->get_mass();

    const float bump_damping    = suspension_settings->get_bump();
    const float rebound_damping = suspension_settings->get_rebound();

    const float critical_damp = Math::sqrt(spring_rate * get_mass() / 4.0f);

    // Pick damping based on compression vs extension
    // Positive spring_velocity = compressing = bump
    // Negative spring_velocity = extending   = rebound
    const float damping_coeff = (p_wheel.spring_velocity >= 0.0f)
        ? bump_damping * critical_damp
        : rebound_damping * critical_damp;

    const float f_net = unsprung_mass * g_along_spring
        - spring_rate * p_wheel.spring_displacement
        - damping_coeff * p_wheel.spring_velocity;

    p_wheel.spring_velocity += (f_net / unsprung_mass) * p_delta;
    p_wheel.spring_displacement += p_wheel.spring_velocity * p_delta;
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
        DebugOverlay::sphere(world_attachment_point + world_wheel_direction * suspension_settings->get_rest(), 0.1f, wheel_data.hit ? Color(1.0f, 1.0f, 0.0f) : Color(1.0f, 0.0f, 0.0f), false);
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

void LNVehicle::set_throttle_percentage(float p_throttle_percentage) {
    input.throttle = p_throttle_percentage;
}

void LNVehicle::set_clutch_percentage(float p_clutch_precentage) {
    input.clutch = p_clutch_precentage;
}

void LNVehicle::request_gear_up() {
    input.gear += 1;
}

void LNVehicle::request_gear_down() {
    input.gear -= 1;
}

int LNVehicle::get_current_gear() const {
    return drivetrain->get_current_gear();
}

float LNVehicle::get_wheel_slip_angle(LNVehicleWheelPosition p_wheel) const {
    ERR_FAIL_INDEX_V(p_wheel, wheels.size(), 0.0f);
    return wheels[p_wheel].slip_angle;
}

float LNVehicle::get_wheel_slip_ratio(LNVehicleWheelPosition p_wheel) const {
    ERR_FAIL_INDEX_V(p_wheel, wheels.size(), 0.0f);
    return wheels[p_wheel].slip_ratio;
}

float LNVehicle::get_engine_torque() const {
    return engine->get_output_torque();
}

float LNVehicle::get_engine_rpm() const {
    return engine->get_rpm();
}

void LNVehicle::_ready() {
    std::feclearexcept(FE_INVALID | FE_OVERFLOW | FE_DIVBYZERO);
    engine->set_audio_stream_player(audio_stream_player);
    engine->set_rpm(1000.0f);

    float longitudinal_stiffness = 10000.0f;  // Cs — N per unit slip ratio
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
    fa->store_line("]");
}

LNVehicle::LNVehicle() {
    set_linear_damp_mode(DAMP_MODE_REPLACE);
    set_linear_damp(0.0f);
    engine.instantiate();
    drivetrain.instantiate();
}
