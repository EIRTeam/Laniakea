#pragma once

#include "debug/debug_overlay.h"
#include "godot_cpp/classes/geometry3d.hpp"
#include "godot_cpp/classes/input.hpp"
#include "godot_cpp/classes/node3d.hpp"
#include "godot_cpp/core/math.hpp"
#include "godot_cpp/variant/plane.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "math.h"

using namespace godot;
class SuspensionTest : public Node3D {
    GDCLASS(SuspensionTest, Node3D);
public:

    struct MacPherson {
        Vector3 strut_top_position;
        Vector3 inner_front_attachment;
        Vector3 inner_rear_attachment;
        Vector3 outer_attachment;
        float toe = 0.0f;
        float static_toe = 0.0f;
        float sai_offset = Math::deg_to_rad(25.0f);
        float min_strut_length = 0.26f;
        float max_strut_length = 0.4f;
    };

    Vector3 get_forward_world() {
        return get_global_basis().xform(Vector3(0.0, 0.0, -1.0f));
    }

    Vector3 get_right_world() {
        return get_global_basis().xform(Vector3(1.0, 0.0, 0.0));
    }

    struct WheelFrameOut {
        Vector3 steering_axis_up_world;
        Vector3 steering_axis_right_world;
        Vector3 steering_axis_forward_world;
    };

    float steer = 0.0f;

    Vector2 to_plane_2d(const Vector3& V, const Vector3& Origin, const Vector3& AxisRight, const Vector3& AxisUp)
    {
        // Convert to plane-local coordinates by projecting the relative vector onto the plane basis.
        // Assumes AxisRight/AxisUp form an orthonormal basis (or close enough).
        const Vector3 Rel = V - Origin;
        return Vector2(
            Rel.dot(AxisRight),
            Rel.dot(AxisUp));
    }

    Vector3 from_plane_2d(const Vector2& V, const Vector3& Origin, const Vector3& AxisRight, const Vector3& AxisUp)
{
	// Reconstruct 3D point from plane coordinates using the provided basis.
	return Origin + (AxisRight * V.x + AxisUp * V.y);
}

    bool solve_ball_joint_location(const Vector3 &p_upper_shock_mount_world, const float &p_control_arm_length, const float &p_strut_length, const Vector3 &p_pivot_axis_proj, const Vector3 &p_pivot_axis_right, const Vector3 &p_pivot_axis_up, Vector3 &p_r_out_intersection) {
        // ---- Planar 2D solve ----------------------------------------------------
        // We solve in a 2D plane defined by (ComponentRightWorld, ComponentUpWorld),
        // with the component origin as the 2D origin.
        //
        // Interpretation:
        //  - Circle A: centered at component origin, radius = control arm length
        //  - Circle B: centered at upper shock mount projected into the plane, radius = strut length
        //
        // Their intersection(s) define possible ball joint locations in the mechanism plane.
        const float SideSign = 1.0f;
        
        const Vector2 Component2D = Vector2();
        const Vector2 UpperShockMount2D = to_plane_2d(p_upper_shock_mount_world,
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
        
        if (IntersectionsCount == 2)
        {
            // Two possible solutions: choose the "outboard" one.
            // We define outboard as the intersection with the larger signed X when mirrored by side.
            // SideSign ensures correct selection for left/right.
            const float Outboard1 = IntersectionA2D.x * SideSign;
            const float Outboard2 = IntersectionB2D.x * SideSign;

            const Vector2 Chosen2D = (Outboard1 >= Outboard2) ? IntersectionA2D : IntersectionB2D;
            
            p_r_out_intersection = from_plane_2d(Chosen2D,
                                p_pivot_axis_proj, p_pivot_axis_right, p_pivot_axis_up);
            return true;
        }
        if (IntersectionsCount == 1)
        {
            // Tangent case: a single intersection (mechanism at boundary of feasibility).
            p_r_out_intersection = from_plane_2d(IntersectionA2D,
                                p_pivot_axis_proj, p_pivot_axis_right, p_pivot_axis_up);
            return true;
        }

        // No solution (circles do not intersect).
        p_r_out_intersection = Vector3();
        return false;
    }

    WheelFrameOut build_wheel_frame(const MacPherson &p_settings, Vector3 p_wheel_hub_world, Vector3 steering_axis_world, float p_steer_angle) {
        const Vector3 BaseSteeringAxisWorld = steering_axis_world.normalized();
        const Vector3 BaseSteeringAxisRightWorld = get_forward_world().cross(BaseSteeringAxisWorld).normalized();
        const Vector3 BaseSteeringAxisForwardWorld = BaseSteeringAxisRightWorld.cross(BaseSteeringAxisWorld).normalized();

        const float SAIAngleOffset = p_settings.sai_offset;
        const float SideSign = 1.0f;

        const Vector3 SteeringAxisUpAfterSAIWorld = BaseSteeringAxisWorld.rotated(BaseSteeringAxisForwardWorld, SAIAngleOffset * -SideSign).normalized();
	    const Vector3 SteeringAxisRightAfterSAIWorld = BaseSteeringAxisRightWorld.rotated(BaseSteeringAxisForwardWorld, SAIAngleOffset * -SideSign).normalized();

        
        const float StaticToeDeg = -SideSign * p_settings.toe;
	    const float WheelYawDeg = p_steer_angle + StaticToeDeg;

        return {
            .steering_axis_up_world = SteeringAxisUpAfterSAIWorld.normalized(),
            .steering_axis_right_world = SteeringAxisRightAfterSAIWorld.rotated(-SteeringAxisUpAfterSAIWorld, WheelYawDeg).normalized(),
            .steering_axis_forward_world = BaseSteeringAxisForwardWorld.rotated(-SteeringAxisUpAfterSAIWorld, WheelYawDeg).normalized()
        };
    }
    MacPherson suspension_config = {
        .strut_top_position = Vector3(0.5, 0.5, 0.0),
        .inner_front_attachment = Vector3(0.0, 0.2, -0.15f),
        .inner_rear_attachment = Vector3(0.0, 0.2, 0.15f),
        .outer_attachment = Vector3(0.75f, 0.2f, 0.0),
    };
    MacPherson suspension_data = suspension_config;
    Vector3 debug_plane_origin = Vector3(1.0, 0.0f, 0.0);

    bool do_intersection(Plane p_plane, Vector3 p_wheel_right, Vector3 p_wheel_up, float p_wheel_width, float p_wheel_radius, Vector3 p_wheel_center, float p_muf_width, float p_muf_long, Vector3 &r_intersection) {
        Vector3 right_offset = p_wheel_right * Math::lerp(-p_wheel_width * 0.5f, p_wheel_width * 0.5f, p_muf_width);
        const float SWEEP_RADS = Math::deg_to_rad(30.0f) * 0.5f;
        Vector3 vertical_offset = (-p_wheel_up  * p_wheel_radius).rotated(p_wheel_right, Math::lerp(-SWEEP_RADS, SWEEP_RADS, p_muf_long));
        Vector3 from = p_wheel_center + right_offset;
        Vector3 to = from + vertical_offset;


        Vector3 intersection;
        DebugOverlay::line(from, to, Color("Yellow"));
        if (p_plane.intersects_ray(from, from.direction_to(to), &intersection)) {
            intersection -= right_offset;
            intersection -= vertical_offset;
            r_intersection = intersection;
            return true;
        }

        return false;
    }

    virtual void _physics_process(double p_delta) override {
        const float tire_radius = 0.2f;
        const float tire_width = 0.2f;

        debug_plane_origin.y += (Input::get_singleton()->get_action_strength("ui_up") - Input::get_singleton()->get_action_strength("ui_down")) * 0.25f * p_delta;

        steer = (Input::get_singleton()->get_action_strength("ui_right") - Input::get_singleton()->get_action_strength("ui_left")) * Math::deg_to_rad(45.0f);

        WheelFrameOut frame = build_wheel_frame(suspension_config, suspension_data.outer_attachment, (suspension_data.outer_attachment.direction_to(suspension_config.strut_top_position)), steer);

        DebugOverlay::line(suspension_data.outer_attachment, suspension_data.outer_attachment + frame.steering_axis_right_world * 0.1f, Color("Red").darkened(0.5f));
        DebugOverlay::line(suspension_data.outer_attachment, suspension_data.outer_attachment + frame.steering_axis_forward_world * 0.1f, Color("Blue").darkened(0.5f));
        DebugOverlay::line(suspension_data.outer_attachment, suspension_data.outer_attachment + frame.steering_axis_up_world * 0.1f, Color("Green").darkened(0.5f));

        Vector3 debug_plane_normal = Vector3(0.0, 1.0, 0.0).rotated(Vector3(0.0, 0.0, -1.0f), Math::deg_to_rad(0.0f));
        Vector3 debug_plane_forward = Vector3(0.0, 0.0, -1.0f);
        Vector3 debug_plane_right = debug_plane_normal.cross(debug_plane_forward).normalized();

        Node3D *mi = get_node<Node3D>("MeshInstance3D");
        Vector3 pos = mi->get_global_position();
        pos.y = debug_plane_origin.y;
        mi->set_global_position(pos);

        Plane plane = Plane(debug_plane_normal, debug_plane_origin);
        
        const Vector3 debug_plane_front_left = debug_plane_origin - debug_plane_right * 0.5f + debug_plane_forward * 0.5f;
        const Vector3 debug_plane_front_right = debug_plane_origin + debug_plane_right * 0.5f + debug_plane_forward * 0.5f;
        const Vector3 debug_plane_back_left = debug_plane_origin - debug_plane_right * 0.5f - debug_plane_forward * 0.5f;
        const Vector3 debug_plane_back_right = debug_plane_origin + debug_plane_right * 0.5f - debug_plane_forward * 0.5f;

        const Color plane_preview_color = Color(0.0, 0.0, 0.0);

        DebugOverlay::line(debug_plane_front_left, debug_plane_front_right, plane_preview_color, false);
        DebugOverlay::line(debug_plane_front_right, debug_plane_back_right, plane_preview_color, false);
        DebugOverlay::line(debug_plane_back_right, debug_plane_back_left, plane_preview_color, false);
        DebugOverlay::line(debug_plane_back_left, debug_plane_front_left, plane_preview_color, false);

        DebugOverlay::circle_with_dir(suspension_data.outer_attachment + frame.steering_axis_right_world * tire_width * 0.5f, frame.steering_axis_right_world, tire_radius, Color(1.0, 0.0, 0.0, 0.25f));
        DebugOverlay::circle_with_dir(suspension_data.outer_attachment - frame.steering_axis_right_world * tire_width * 0.5f, frame.steering_axis_right_world, tire_radius, Color(1.0, 0.0, 0.0, 0.25f));

        // Simulate contact 
        const Vector3 inner_rear_to_outer = suspension_config.outer_attachment - suspension_config.inner_rear_attachment;
        const Vector3 inner_rear_to_inner_front = suspension_config.inner_front_attachment - suspension_config.inner_rear_attachment;
        const Vector3 control_arm_proj = suspension_config.inner_rear_attachment + inner_rear_to_outer.project(inner_rear_to_inner_front);
        const Vector3 control_arm_proj_right = control_arm_proj.direction_to(suspension_data.outer_attachment);
        const Vector3 control_arm_proj_up = control_arm_proj.direction_to(suspension_data.outer_attachment).cross(Vector3(0.0, 0.0, -1.0f));

        const Vector2 outer_2d = to_plane_2d(suspension_data.outer_attachment, control_arm_proj, control_arm_proj_right, control_arm_proj_up);
        
        const float control_arm_length_2d = control_arm_proj.distance_to(suspension_config.outer_attachment);

        Vector3 out;

        //UtilityFunctions::prints(control_arm_length_2d, suspension_data.outer_attachment.length());
        //UtilityFunctions::prints(suspension_data.strut_top_position.distance_to(suspension_data.outer_attachment), strut_length_2d);
        float strut_length = suspension_data.max_strut_length;
        Vector3 from = suspension_data.outer_attachment;
        Vector3 to = from - frame.steering_axis_up_world * tire_radius;
        DebugOverlay::line(from, to, Color("Yellow"));

        std::optional<Vector3> highest_hub_position;

        const int cast_count_width = 3;
        const int cast_count_radial = 6;
        for (int i = 0; i < cast_count_width; i++) {
            const float muf_long = i / static_cast<float>(cast_count_width-1);
            for (int j = 0; j < cast_count_radial; j++) {
                Vector3 intersection;
                const float muf_radial = j / static_cast<float>(cast_count_radial-1);
                if (!do_intersection(plane, frame.steering_axis_right_world, frame.steering_axis_up_world, tire_width, tire_radius, suspension_data.outer_attachment, muf_long, muf_radial, intersection)) {
                    continue;
                }

                if (!highest_hub_position.has_value()) {
                    highest_hub_position = intersection;
                    continue;
                }

                if (intersection.y > to_local(*highest_hub_position).y) {
                    highest_hub_position = intersection;
                }
            }
        }

        if (highest_hub_position.has_value()) {
            strut_length = highest_hub_position->distance_to(suspension_config.strut_top_position);
        }

        strut_length = CLAMP(strut_length, suspension_config.min_strut_length, suspension_config.max_strut_length);

        /*if (plane.intersects_ray(from, from.direction_to(to), &intersection)) {
            intersection += frame.steering_axis_up_world * tire_radius;
            strut_length = intersection.distance_to(suspension_data.strut_top_position);
        }*/

        if (solve_ball_joint_location(suspension_data.strut_top_position, control_arm_length_2d, strut_length, control_arm_proj, control_arm_proj_right, control_arm_proj_up, out)) {
            DebugOverlay::sphere(out, 0.01f, Color("GREEN"));
            suspension_data.outer_attachment = out;
        }
        DebugOverlay::line(suspension_config.strut_top_position, suspension_data.outer_attachment, Color("Green"));
        DebugOverlay::line(suspension_config.inner_front_attachment, suspension_data.outer_attachment, Color("Blue"));
        DebugOverlay::line(suspension_config.inner_rear_attachment, suspension_data.outer_attachment, Color("Blue"));
    }

    static void _bind_methods() {}
};