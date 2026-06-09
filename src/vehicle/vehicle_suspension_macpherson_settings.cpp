#include "vehicle_suspension_macpherson_settings.h"
#include "bind_macros.h"
#include "console/console_system.h"
#include "math.h"
#include "vehicle/vehicle.h"
#include "vehicle/vehicle_suspension.h"
#include "vehicle/vehicle_suspension_settings.h"
#include "vehicle/vehicle_suspension_state.h"

LNVehicleSuspensionState* LNVehicleMacPhersonSuspensionSettings::create_suspension_state() const {
    LNVehicleMacPhersonSuspensionState *state = memnew(LNVehicleMacPhersonSuspensionState);
    state->shock_length = get_strut_car().distance_to(get_bottom_wishbone_tyre());
    state->bottom_tyre_ball_joint_suspension_local = get_bottom_wishbone_tyre();
    state->prev_shock_length = state->shock_length;
    return state;
}

void LNVehicleMacPhersonSuspensionSettings::_bind_methods() {
    MAKE_BIND_VECTOR3(LNVehicleMacPhersonSuspensionSettings, bottom_wishbone_front);    
    MAKE_BIND_VECTOR3(LNVehicleMacPhersonSuspensionSettings, bottom_wishbone_rear);
    MAKE_BIND_VECTOR3(LNVehicleMacPhersonSuspensionSettings, bottom_wishbone_tyre);
}

float LNVehicleMacPhersonSuspensionSettings::get_rest_spring_length() const {
    return get_bottom_wishbone_tyre().distance_to(get_strut_car()) + get_rod_length_offset();
}

void LNVehicleMacPhersonSuspensionSettings::_rebuild_cache() {
    Transform3D suspension_trf = Transform3D();
    
    LNVehicleSuspension::WheelFrameOut test_frame = LNVehicleSuspension::build_wheel_frame(0.0f, 0.0f, Transform3D(), 1.0f, 0.0f, get_toe_out(), get_bottom_wishbone_tyre().direction_to(get_strut_car()), 0.0f);
    
    Transform3D original_steering_trf = Transform3D();
    original_steering_trf.basis = Basis(test_frame.steering_axis_right_world, test_frame.steering_axis_up_world, test_frame.steering_axis_forward_world);
    original_steering_trf.origin = suspension_trf.xform(get_bottom_wishbone_tyre());

    Transform3D hub_trf = Transform3D();
    hub_trf.basis = Basis(test_frame.steering_axis_right_world, test_frame.steering_axis_up_world, test_frame.steering_axis_forward_world);
    hub_trf.origin = suspension_trf.xform(Vector3());


    const Plane camber_plane = Plane(Vector3(0, 0, 1)); // world forward as normal
    Vector3 axis_camber_plane = camber_plane.project(test_frame.steering_axis_up_world).normalized();
    Vector3 up_camber_plane = Vector3(0, 1, 0); // world Y needs no projection, it's already in the plane

    const float design_camber = up_camber_plane.signed_angle_to(
        axis_camber_plane,
        Vector3(0, 0, 1)
    );

    const Plane world_horizontal = Plane(Vector3(0, 1, 0));
    Vector3 wheel_fwd_flat = world_horizontal.project(test_frame.steering_axis_forward_world).normalized();
    Vector3 world_fwd_flat = Vector3(0, 0, 1); // your car's forward

    // Angle to rotate the steering axis around its own up to zero the toe
    const float design_toe = wheel_fwd_flat.signed_angle_to(
        world_fwd_flat,
        test_frame.steering_axis_up_world  // rotate around hub up so camber is preserved
    );

    macpherson_cache = MacPhersonCache {
        .design_camber = design_camber,
        .design_toe = design_toe,
        .bottom_balljoint_hub_local = hub_trf.affine_inverse().xform(suspension_trf.xform(get_bottom_wishbone_tyre())),
        .hub_wheel_local = original_steering_trf.affine_inverse().xform(Vector3())
    };

    const Vector3 inner_rear_attachment_suspension_space = get_bottom_wishbone_rear();
    const Vector3 inner_front_attachment_suspension_space = get_bottom_wishbone_front();
    const Vector3 balljoint_suspension_space = get_bottom_wishbone_tyre();

    const Vector3 inner_rear_to_outer = (balljoint_suspension_space - inner_rear_attachment_suspension_space);
    const Vector3 inner_rear_to_inner_front = (inner_front_attachment_suspension_space - inner_rear_attachment_suspension_space);
    const Vector3 control_arm_proj = inner_rear_attachment_suspension_space + inner_rear_to_outer.project(inner_rear_to_inner_front);
    const Vector3 control_arm_proj_right = balljoint_suspension_space.direction_to(control_arm_proj);
    const Vector3 control_arm_proj_up = control_arm_proj_right.cross(inner_rear_to_inner_front.normalized());

    macpherson_cache->pivot_center = control_arm_proj;
    macpherson_cache->pivot_axis_right = control_arm_proj_right;
    macpherson_cache->pivot_axis_up = control_arm_proj_up;

    macpherson_cache->bottom_control_arm_length_2d = control_arm_proj.distance_to(balljoint_suspension_space);
    // Find minimum and maximum strut length


    float design_0_length = get_strut_car().distance_to(get_bottom_wishbone_tyre());

    float maximum_test = get_strut_car().distance_to(control_arm_proj) + macpherson_cache->bottom_control_arm_length_2d;
    float minimum_test = maximum_test - macpherson_cache->bottom_control_arm_length_2d * 2.0f;

    const float total_vertical_travel = maximum_test - minimum_test;

    const float step_granularity = 0.002f;

    int steps = Math::ceil(total_vertical_travel / step_granularity);

    std::optional<float> minimum_shock;
    std::optional<float> maximum_shock;

    for (int i = 0; i < steps; i++) {
        const float extension = Math::lerp(minimum_test, maximum_test, (i+1) / static_cast<float>(steps));
        Vector3 intersection;
        if (solve_ball_joint_location(get_strut_car(), 1.0f, macpherson_cache->bottom_control_arm_length_2d, extension, control_arm_proj, control_arm_proj_right, control_arm_proj_up, intersection)) {
            if (!minimum_shock.has_value()) {
                minimum_shock = extension;
            }

            maximum_shock = extension;
        } else if (maximum_shock.has_value()) {
            break;
        } else {
        }
    }

    DEV_ASSERT(minimum_shock.has_value());
    DEV_ASSERT(maximum_shock.has_value());

    macpherson_cache->strut_extension_min = *minimum_shock;
    macpherson_cache->strut_extension_max = *maximum_shock;
}

bool LNVehicleMacPhersonSuspensionSettings::solve_ball_joint_location(const Vector3 &p_upper_shock_mount_world, float p_side_sign, const float &p_control_arm_length, const float &p_strut_length, const Vector3 &p_pivot_axis_proj, const Vector3 &p_pivot_axis_right, const Vector3 &p_pivot_axis_up, Vector3 &p_r_out_intersection) {
    // ---- Planar 2D solve ----------------------------------------------------
    // We solve in a 2D plane defined by (ComponentRightWorld, ComponentUpWorld),
    // with the component origin as the 2D origin.
    //
    // Interpretation:
    //  - Circle A: centered at component origin, radius = control arm length
    //  - Circle B: centered at upper shock mount projected into the plane, radius = strut length
    //
    // Their intersection(s) define possible ball joint locations in the mechanism plane.
    const Vector2 Component2D = Vector2();
    const Vector2 UpperShockMount2D = LNMath::to_plane_2d(p_upper_shock_mount_world,
        p_pivot_axis_proj, p_pivot_axis_right, p_pivot_axis_up);

    Vector2 IntersectionA2D;
    Vector2 IntersectionB2D;
    const int32_t IntersectionsCount = LNMath::circle_intersect(Component2D,
                                                    p_control_arm_length,
                                                    UpperShockMount2D,
                                                    p_strut_length,
                                                IntersectionA2D,
                                                IntersectionB2D,
                                                1e-4f);
    

    Vector3 fwd = p_pivot_axis_up.cross(p_pivot_axis_right).normalized();
    const Vector3 proj = LNMath::from_plane_2d(Component2D, p_pivot_axis_proj, p_pivot_axis_right, p_pivot_axis_up);
    //DebugOverlay::circle_with_dir(proj, fwd, p_control_arm_length, Color(0.0, 1.0, 0.0, 0.25f));
    //DebugOverlay::circle_with_dir(LNMath::from_plane_2d(UpperShockMount2D, p_pivot_axis_proj, p_pivot_axis_right, p_pivot_axis_up), fwd, p_strut_length, Color(0.0, 0.0, 1.0, 0.25f));

    if (IntersectionsCount == 2)
    {
        // Two possible solutions: choose the "outboard" one.
        // We define outboard as the intersection with the larger signed X when mirrored by side.
        // SideSign ensures correct selection for left/right.
        const float Outboard1 = IntersectionA2D.x * (p_side_sign);
        const float Outboard2 = IntersectionB2D.x * (p_side_sign);

        const Vector2 Chosen2D = (Outboard1 <= Outboard2) ? IntersectionA2D : IntersectionB2D;
        
        p_r_out_intersection = LNMath::from_plane_2d(Chosen2D,
                            p_pivot_axis_proj, p_pivot_axis_right, p_pivot_axis_up);
        return true;
    }
    if (IntersectionsCount == 1)
    {
        // Tangent case: a single intersection (mechanism at boundary of feasibility).
        p_r_out_intersection = LNMath::from_plane_2d(IntersectionA2D,
                            p_pivot_axis_proj, p_pivot_axis_right, p_pivot_axis_up);
        return true;
    }

    // No solution (circles do not intersect).
    p_r_out_intersection = Vector3();
    return false;
}

LNVehicleSuspensionSettings::SuspensionSolveResult LNVehicleMacPhersonSuspensionSettings::solve(Ref<LNVehicleWheelSettings> p_wheel_settings, LNVehicleWheelPosition p_wheel_position, const Node3D *p_vehicle, double p_delta, LNVehicleSuspensionState *p_state) {
    /*if (p_wheel_position == WHEEL_FR || p_wheel_position == WHEEL_RR) {
        return {
            .success = true,
            .grounded = false
        };
    }*/
    LNVehicleMacPhersonSuspensionState *state = dynamic_cast<LNVehicleMacPhersonSuspensionState*>(p_state);

    if (!macpherson_cache.has_value()) {
        _rebuild_cache();
    }

    DEV_ASSERT(state != nullptr);

    const float side_sign = p_wheel_position == WHEEL_FL || p_wheel_position == WHEEL_RL ? 1.0 : -1.0f;

    const Transform3D vehicle_transform = p_vehicle->get_global_transform();

    const Vector3 bottom_tyre_ball_joint_world = state->suspension_transform_world.xform(state->bottom_tyre_ball_joint_suspension_local);
    const Vector3 strut_attachment_world = state->suspension_transform_world.xform(get_strut_car());

    LNVehicleSuspension::WheelFrameOut wheel_frame = LNVehicleSuspension::build_wheel_frame(macpherson_cache->design_camber, macpherson_cache->design_toe, vehicle_transform, side_sign, Math::deg_to_rad(get_static_camber_degrees()), 0.0f, bottom_tyre_ball_joint_world.direction_to(strut_attachment_world), state->steering_angle_rads);

    DebugOverlay::line(bottom_tyre_ball_joint_world, bottom_tyre_ball_joint_world + wheel_frame.steering_axis_right_world * 0.1f, Color("Red").darkened(0.5f));
    DebugOverlay::line(bottom_tyre_ball_joint_world, bottom_tyre_ball_joint_world + wheel_frame.steering_axis_forward_world * 0.1f, Color("Blue").darkened(0.5f));
    DebugOverlay::line(bottom_tyre_ball_joint_world, bottom_tyre_ball_joint_world + wheel_frame.steering_axis_up_world * 0.1f, Color("Green").darkened(0.5f));

    /*DebugOverlay::line(bottom_tyre_ball_joint_world, bottom_tyre_ball_joint_world + wheel_frame.wheel_trf_out.xform(Vector3(1.0, 0.0, 0.0)) * 0.1f, Color("Red"));
    DebugOverlay::line(bottom_tyre_ball_joint_world, bottom_tyre_ball_joint_world + wheel_frame.wheel_trf_out.xform(Vector3(0.0, 0.0, -1.0)) * 0.1f, Color("Blue"));
    DebugOverlay::line(bottom_tyre_ball_joint_world, bottom_tyre_ball_joint_world + wheel_frame.wheel_trf_out.xform(Vector3(0.0, 1.0, 0.0)) * 0.1f, Color("Green"));*/

    const float tire_width = p_wheel_settings->get_width();
    const float tire_radius = p_wheel_settings->get_radius();

    const Vector3 inner_rear_attachment_world = state->suspension_transform_world.xform(get_bottom_wishbone_rear());
    const Vector3 inner_front_attachment_world = state->suspension_transform_world.xform(get_bottom_wishbone_front());

    DebugOverlay::filled_arrow(strut_attachment_world, bottom_tyre_ball_joint_world, 0.05f, Color("GREEN"));
    
    const Vector3 control_arm_proj = p_state->suspension_transform_world.xform(macpherson_cache->pivot_center);
    const Vector3 control_arm_proj_right = p_state->suspension_transform_world.basis.xform(macpherson_cache->pivot_axis_right);
    const Vector3 control_arm_proj_up = p_state->suspension_transform_world.basis.xform(macpherson_cache->pivot_axis_up);

    const float control_arm_length_2d = macpherson_cache->bottom_control_arm_length_2d;

    Vector3 out;

    float strut_length = p_state->shock_length;

    std::optional<LNVehicleSuspension::WheelIntersectionResult> highest_hit_world;

    const int cast_count_width = 3;
    const int cast_count_radial = 6;

    Transform3D wheel_trf =  Transform3D(
        Basis(
            side_sign * wheel_frame.steering_axis_right_world,
            wheel_frame.steering_axis_up_world,
            wheel_frame.steering_axis_forward_world
        ),
        bottom_tyre_ball_joint_world
    );

    Vector3 hub_pos_world = wheel_trf.xform(macpherson_cache->hub_wheel_local);

    for (int i = 0; i < cast_count_width; i++) {
        const float muf_long = i / static_cast<float>(cast_count_width-1);
        for (int j = 0; j < cast_count_radial; j++) {
            const float muf_radial = j / static_cast<float>(cast_count_radial-1);
            LNVehicleSuspension::WheelIntersectionResult intersection_result = LNVehicleSuspension::do_wheel_intersection(p_vehicle, (-side_sign) * wheel_frame.steering_axis_right_world, wheel_frame.steering_axis_up_world, tire_width, tire_radius, hub_pos_world, muf_long, muf_radial);
            if (!intersection_result.hit) {
                continue;
            }

            if (!highest_hit_world.has_value()) {
                highest_hit_world = intersection_result;
                continue;
            }

            if (p_vehicle->to_local(intersection_result.wheel_center_position).y > p_vehicle->to_local(highest_hit_world->wheel_center_position).y) {
                highest_hit_world = intersection_result;
            }
        }
    }

    if (highest_hit_world.has_value()) {
        Transform3D hub_trf;
        hub_trf.basis = Basis(
            side_sign * wheel_frame.steering_axis_right_world,
            wheel_frame.steering_axis_up_world,
            wheel_frame.steering_axis_forward_world
        );
        hub_trf.origin = highest_hit_world->wheel_center_position;
        Vector3 new_balljoint_pos = hub_trf.xform(macpherson_cache->bottom_balljoint_hub_local);
        strut_length = strut_attachment_world.distance_to(new_balljoint_pos);
        strut_length = CLAMP(strut_length, macpherson_cache->strut_extension_min, macpherson_cache->strut_extension_max);
    }

    // Spring forces
    
    LNVehicleSuspension::SuspensionForceResult force_result;

    if (highest_hit_world.has_value()) {
        state->prev_shock_length = state->shock_length;
        force_result = LNVehicleSuspension::compute_suspension_force(this, true, get_rest_spring_length(), state->prev_shock_length, strut_length, p_delta);
        if (force_result.spring_force < 0.0f) {
            highest_hit_world.reset();
        }
    } else if (!highest_hit_world.has_value()) {
        // wheel is in the air, extend it
        // spring length has not changed in this frame since we didn't get a hit, so we will use state->prev_strut_length to estimate the velocity
        force_result = LNVehicleSuspension::compute_suspension_force(this, false, get_rest_spring_length(), state->prev_shock_length, p_state->shock_length, p_delta);
    }

    if (!highest_hit_world.has_value()) {
        float gravity_force = p_wheel_settings->get_mass() * 9.81f * bottom_tyre_ball_joint_world.direction_to(strut_attachment_world).dot(Vector3(0, -1, 0));
        float net_force = force_result.clamped_total_force - gravity_force;
        float suspension_acceleration = net_force / p_wheel_settings->get_mass();
        float new_strut_length = p_state->shock_length + force_result.velocity * p_delta + 0.5f * suspension_acceleration * p_delta * p_delta;
        strut_length = CLAMP(new_strut_length, macpherson_cache->strut_extension_min, macpherson_cache->strut_extension_max);
        state->prev_shock_length = state->shock_length;
        state->shock_length = strut_length;
    }
    
    // Solve ball joint location

    Transform3D suspension_trf_inv = state->suspension_transform_world.affine_inverse();
    Vector3 strut_attachment_suspension_space = get_strut_car();

    if (solve_ball_joint_location(strut_attachment_suspension_space, 1.0f, control_arm_length_2d, strut_length, macpherson_cache->pivot_center, macpherson_cache->pivot_axis_right, macpherson_cache->pivot_axis_up, out)) {
        float prev_shock_length = state->shock_length;
        state->bottom_tyre_ball_joint_suspension_local = out;

        Transform3D new_steering_trf = Transform3D(
            Basis(
                side_sign * wheel_frame.steering_axis_right_world,
                wheel_frame.steering_axis_up_world,
                wheel_frame.steering_axis_forward_world
            ),
            p_state->suspension_transform_world.xform(state->bottom_tyre_ball_joint_suspension_local)
        );

        hub_pos_world = new_steering_trf.xform(macpherson_cache->hub_wheel_local);
        state->shock_length = strut_length;

        const Vector3 wishbone_front = state->suspension_transform_world.xform(get_bottom_wishbone_front());
        const Vector3 wishbone_rear = state->suspension_transform_world.xform(get_bottom_wishbone_rear());

        DebugOverlay::filled_arrow(wishbone_front, state->suspension_transform_world.xform(state->bottom_tyre_ball_joint_suspension_local), 0.05f, Color("BLUE"));
        DebugOverlay::filled_arrow(wishbone_rear, state->suspension_transform_world.xform(state->bottom_tyre_ball_joint_suspension_local), 0.05f, Color("RED"));

        // Grounded
        if (highest_hit_world.has_value()) {
            Vector3 ShockAxis  = p_state->suspension_transform_world.xform(state->bottom_tyre_ball_joint_suspension_local).direction_to(strut_attachment_world);
            const float Cos = MAX(0.0f, ShockAxis.dot(highest_hit_world->ground_normal));
            const Vector3 force_to_apply = highest_hit_world->ground_normal * (force_result.clamped_total_force * Cos);

            return {
                .success = true,
                .grounded = true,
                .force_to_apply = force_to_apply,
                .force_world_position = highest_hit_world->ground_hit_position,
                .wheel_transform = Transform3D(new_steering_trf.basis, hub_pos_world),
                .grounded_normal = highest_hit_world->ground_normal,
                .ground_hit_position = highest_hit_world->ground_hit_position
            };
        }
    }

    return {
        .success = true,
        .grounded = false
    };
}

