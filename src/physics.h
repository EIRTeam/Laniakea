#pragma once

#include "godot_cpp/classes/rigid_body3d.hpp"
#include <optional>
#include "debug/debug_overlay.h"

using namespace godot;

namespace LNPhysics {
    _FORCE_INLINE_ Vector3 velocity_at_pos(RigidBody3D *p_body, Vector3 p_world_pos) {
        return p_body->get_linear_velocity() + p_body->get_angular_velocity().cross(p_world_pos - p_body->get_global_position());
    }
    _FORCE_INLINE_ void apply_force(RigidBody3D *p_body, Vector3 p_force_global, Vector3 p_offset_global, std::optional<Color> p_color = {}) {
        DEV_ASSERT(p_force_global.is_finite());
        if (const Vector3 force_visual = p_force_global / 500.0f; !force_visual.is_zero_approx()) {
            Color color = p_color.value_or(Color(0.0, 1.0, 1.0));
            DebugOverlay::filled_arrow(p_body->get_global_position()+p_offset_global, p_body->get_global_position()+p_offset_global+force_visual, 0.15f, color, false);
        }
        p_body->apply_force(p_force_global, p_offset_global);
    }
}
