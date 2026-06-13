#include "base_movement.h"
#include "debug/debug_constexpr.h"
#include "debug/debug_overlay.h"
#include "game/movement_shared.h"
#include "godot_cpp/classes/cylinder_shape3d.hpp"
#include "godot_cpp/classes/physics_server3d.hpp"
#include "godot_cpp/classes/physics_test_motion_parameters3d.hpp"
#include "godot_cpp/classes/physics_test_motion_result3d.hpp"
#include "godot_cpp/classes/world3d.hpp"
#include "godot_cpp/core/math.hpp"
#include "godot_cpp/variant/transform3d.hpp"
#include "physics_layers.h"
#include "springs.h"

bool BaseMovement::_test_move(const Transform3D &p_trf, const Vector3 &p_motion, Ref<PhysicsTestMotionResult3D> &r_result, float p_margin) const {
    Ref<PhysicsTestMotionParameters3D> params;
    params.instantiate();
    params->set_from(p_trf);
    params->set_motion(p_motion);
    params->set_margin(p_margin);
    return PhysicsServer3D::get_singleton()->body_test_motion(body, params, r_result);
}

void BaseMovement::initialize(Ref<MovementSettings> p_movement_settings, Node3D *p_owner) {
    movement_settings = p_movement_settings;

    PhysicsServer3D *ps = PhysicsServer3D::get_singleton();
    body = ps->body_create();
    owner_node = p_owner;
    ps->body_set_mode(body, PhysicsServer3D::BODY_MODE_KINEMATIC);
    ps->body_attach_object_instance_id(body, owner_node->get_instance_id());
    ps->body_set_space(body, owner_node->get_world_3d()->get_space());
    const float radius = p_movement_settings->get_radius();
    for (int i = 0; i < Movement::STANCE_MAX; i++) {
        Ref<CylinderShape3D> cs;
        cs.instantiate();
        cs->set_radius(radius);
        cs->set_height(p_movement_settings->get_stance_height(i));
        ps->body_add_shape(body, cs->get_rid());
        ps->body_set_shape_disabled(body, i, i != 0);
        body_shapes_per_stance[i] = cs;
    }

    desired_velocity_spring.initialize(p_movement_settings->get_movement_halflife());

    ps->body_set_collision_layer(body, PhysicsLayers::LAYER_ENTITY_MOVEMENT_BOXES);
    ps->body_set_collision_mask(body, PhysicsLayers::LAYER_WORLDSPAWN | PhysicsLayers::LAYER_PROPS);
}

void BaseMovement::update(float p_delta) {

    desired_velocity_spring.update(get_desired_velocity(), p_delta);

    const Vector3 prev_pos = owner_node->get_global_position();

    PhysicsServer3D::get_singleton()->body_set_state(body, PhysicsServer3D::BODY_STATE_TRANSFORM, owner_node->get_global_transform());

    MovementPassResult lateral_pass_result;
    MovementPassResult gravity_pass_result;
    _do_movement_pass(MovementPass::LATERAL, p_delta, lateral_pass_result);
    _do_movement_pass(MovementPass::GRAVITY, p_delta, gravity_pass_result);
    bool did_movement_snap = lateral_pass_result.movement_snapped;
    if (gravity_pass_result.hit_something) {
        grounded = true;
    } else if (!get_smoothed_desired_velocity().is_zero_approx()) {
        if (grounded) {
            grounded = false;
            MovementPassResult snap_pass_result;
            _do_movement_pass(MovementPass::SNAP, p_delta, snap_pass_result);

            if (snap_pass_result.hit_something) {
                did_movement_snap = true;
                grounded = true;
            }
        }
    }

    if (grounded) {
        vertical_velocity = 0.0f;
    }

    if (did_movement_snap) {
        // TODO: add movement snapped callback
    }

    effective_velocity = (owner_node->get_global_position() - prev_pos) / p_delta;

    if constexpr (Debug::is_debug_enabled) {
        constexpr int PATH_PREDICTION_STEPS = 16;
        constexpr float PATH_PREDICTION_TIME = 3.0f;

        PackedVector3Array predicted_path = desired_velocity_spring.predict(owner_node->get_global_position(), get_desired_velocity(), PATH_PREDICTION_TIME, PATH_PREDICTION_STEPS);
        DebugOverlay::path( predicted_path, true, Color::named("green"));
    }
}

void BaseMovement::_handle_collision(const Vector3 &p_velocity, const Ref<PhysicsTestMotionResult3D> &p_collision_result) {
    PhysicsServer3D *ps = PhysicsServer3D::get_singleton();
    const float our_mass = movement_settings->get_push_mass();

    for (int i = 0; i < p_collision_result->get_collision_count(); i++) {
        RID other_body_rid = p_collision_result->get_collider_rid(i);
        const bool is_rigidbody = ps->body_get_mode(other_body_rid) == PhysicsServer3D::BODY_MODE_RIGID || ps->body_get_mode(other_body_rid) == PhysicsServer3D::BODY_MODE_RIGID_LINEAR;
        if (!is_rigidbody) {
            continue;
        }
        // Apply force to rigibodies when we hit any
        const Transform3D body_trf = ps->body_get_state(other_body_rid, PhysicsServer3D::BODY_STATE_TRANSFORM);
        const Vector3 body_pos = body_trf.origin;
        const Vector3 collision_normal = p_collision_result->get_collision_normal(i);
        const float vel_along_normal = p_velocity.dot(-collision_normal);
        const Vector3 collision_point_relative = p_collision_result->get_collision_point(i) - body_pos;
        const Vector3 force = (vel_along_normal * (-collision_normal)) * our_mass;
        DebugOverlay::horz_arrow(p_collision_result->get_collision_point(i), p_collision_result->get_collision_point(i) + force.normalized(), 0.25f, Color::named("RED"), false, 0.25f);
		// TODO: Do we need to multiply our_mass here again?
        ps->body_apply_force(other_body_rid, force * our_mass, collision_point_relative);
    }
}

Vector3 BaseMovement::get_smoothed_desired_velocity() const {
    return desired_velocity_spring.get();
}

Vector3 BaseMovement::get_effective_velocity() const {
    return effective_velocity;
}

Movement::MovementStance BaseMovement::get_current_stance() const {
    return current_stance;
}

void BaseMovement::set_enabled(bool p_enabled) {

    PhysicsServer3D *ps = PhysicsServer3D::get_singleton();

    for (int i = 0; i < ps->body_get_shape_count(body); i++) {
        ps->body_set_shape_disabled(body, 0, !p_enabled);
    }
}

void BaseMovement::add_collision_exception(RID p_body) {
    PhysicsServer3D::get_singleton()->body_add_collision_exception(body, p_body);
}

void BaseMovement::remove_collision_exception(RID p_body) {
    PhysicsServer3D::get_singleton()->body_remove_collision_exception(body, p_body);
}

BaseMovement::~BaseMovement() {
    if (body.is_valid()) {
        PhysicsServer3D::get_singleton()->free_rid(body);
    }
}

void BaseMovement::_try_snap_up_stair(const Vector3 &p_movement_dir, const Transform3D &p_starting_trf, StairSnapResult &r_result) const {
    Ref<PhysicsTestMotionResult3D> stair_up_collision;
    stair_up_collision.instantiate();
    const float MAX_STEP_HEIGHT = movement_settings->get_max_step_height();
    const float MING_STEP_DEPTH = movement_settings->get_min_step_depth();

    // Try casting up
    stair_up_collision.instantiate();

    if (_test_move(p_starting_trf, Vector3(0.0f, MAX_STEP_HEIGHT, 0.0f), stair_up_collision)) {
        r_result = {
            .snapped = false
        };
        return;
    }

    // Try casting to new stair location
    Ref<PhysicsTestMotionResult3D> stair_down_collision;
    stair_down_collision.instantiate();
    Transform3D cast_down_origin = p_starting_trf;
    cast_down_origin.origin += Vector3(0.0f, MAX_STEP_HEIGHT, 0.0f);

    if (!_test_move(cast_down_origin, Vector3(0.0f, -MAX_STEP_HEIGHT, 0.0f) + p_movement_dir * MING_STEP_DEPTH, stair_down_collision)) {
        r_result = {
            .snapped = false
        };
        return;
    }

    // Finally, check if the target surface is walkable
    const float motion_angle_rad = Math::acos(stair_down_collision->get_collision_normal().dot(Vector3(0.0f, 1.0f, 0.0f)));
    if (motion_angle_rad > movement_settings->get_max_slope_angle()) {
        r_result = {
            .snapped = false
        };
        return;
    }

    r_result = {
        .snapped = true,
        .resulting_position = cast_down_origin.origin + stair_down_collision->get_travel()
    };
}

void BaseMovement::_do_movement_pass(const MovementPass p_pass, float p_delta, MovementPassResult &r_out) {
    Transform3D current_trf = owner_node->get_global_transform();
    Vector3 remaining_motion = {};
    Vector3 desired_input = {};

    const float SNAP_TO_GROUND_HEIGHT = movement_settings->get_snap_to_ground_height();
    const float TERMINAL_VELOCITY = movement_settings->get_terminal_velocity();
    const float GRAVITY_ACCEL = movement_settings->get_gravity();

    switch (p_pass) {
		case LATERAL: {
            desired_input = get_smoothed_desired_velocity().normalized();
            remaining_motion = get_smoothed_desired_velocity() * p_delta;
        } break;
		case GRAVITY: {
            vertical_velocity = Math::move_toward(vertical_velocity, -TERMINAL_VELOCITY, GRAVITY_ACCEL * p_delta);

            float desired_downward_motion = vertical_velocity * p_delta;
            // Ensure we always probe at least a small amount downward
            // this guarantees reliable ground detection even when vertical_velocity ≈ 0
            constexpr float GROUND_PROBE_MARGIN = 0.001f;
            remaining_motion.y = Math::min(desired_downward_motion, -GROUND_PROBE_MARGIN);

        } break;
		case SNAP: {
            remaining_motion = Vector3(0.0f, -SNAP_TO_GROUND_HEIGHT, 0.0f);
        } break;
	}

    // TODO: Check if we really need 3 iterations to solve this
    for (int i = 0; i < 3; i++) {
        MovementIterResult iter_result;
        _movement_iter({
            .starting_trf = current_trf,
            .motion = remaining_motion,
            .desired_movement_direction = desired_input,
            .movement_pass = p_pass
        }, iter_result);
        if (iter_result.hit_something && p_pass == MovementPass::LATERAL) {
            _handle_collision(desired_input, iter_result.kinematic_collision_result);
        }

        current_trf = iter_result.new_transform;
        remaining_motion = iter_result.remaining_velocity;
        r_out.movement_snapped = r_out.movement_snapped || iter_result.movement_snapped;
        r_out.hit_something = r_out.hit_something  || iter_result.hit_something;
        if (iter_result.done) {
            break;
        }
    }

    if (p_pass != MovementPass::SNAP || r_out.hit_something) {
        owner_node->set_global_transform(current_trf);
    }
}

void BaseMovement::set_input_vector(const Vector2 &p_input_vector) {
    input_vector = p_input_vector;
}

void BaseMovement::set_desired_movement_speed(Movement::MovementSpeed p_movement_speed) {
    desired_movement_speed = p_movement_speed;
}

Vector3 BaseMovement::get_desired_velocity() const {
    const Vector3 input_vector_3d = Vector3(input_vector.x, 0.0f, input_vector.y);
    float max_speed = 0.0f;

    if (desired_movement_speed == Movement::MovementSpeed::SPRINTING) {
        max_speed = movement_settings->get_stance_max_sprint_speed(get_current_stance());
    } else if (desired_movement_speed == Movement::MovementSpeed::RUNNING) {
        max_speed = movement_settings->get_stance_max_run_speed(get_current_stance());
    } else if (desired_movement_speed == Movement::MovementSpeed::WALKING) {
        max_speed = movement_settings->get_stance_max_walk_speed(get_current_stance());
    }
    return max_speed * input_vector_3d.normalized();
}

Vector3 _FORCE_INLINE_ _plane_project(const Vector3 &p_normal, const Vector3 &p_velocity) {
	Vector3 out = Plane(p_normal).project(p_velocity).normalized();
	out *= p_velocity.length();
	return out;
}

void BaseMovement::_movement_iter(const MovementIterParams &p_params, MovementIterResult &r_out) const {
    Ref<PhysicsTestMotionResult3D> shape_cast_result;
    shape_cast_result.instantiate();
    r_out.new_transform = p_params.starting_trf;
    if (!_test_move(p_params.starting_trf, p_params.motion, shape_cast_result, p_params.motion_margin)) {
        r_out.done = true;
        r_out.new_transform.origin += p_params.motion;
        return;
    }

    r_out.kinematic_collision_result = shape_cast_result;
    r_out.hit_something = true;

    DEV_ASSERT(shape_cast_result.is_valid());

    Vector3 travel = shape_cast_result->get_travel();
    Vector3 remainder = shape_cast_result->get_remainder();

	// We travelled a bit too little, so let's pretend we didn't travel at all
    if (travel.length() < p_params.motion_margin) {
        travel = Vector3();
    }

    r_out.new_transform.origin = p_params.starting_trf.origin + travel;

    const Vector3 cast_normal = shape_cast_result->get_collision_normal();
    const float angle = cast_normal.angle_to(Vector3(0.0f, 1.0f, 0.0f));

    const float MAX_SLOPE_ANGLE = movement_settings->get_max_slope_angle();

    if (angle <= MAX_SLOPE_ANGLE) {
        remainder = _plane_project(cast_normal, remainder);
        // Prevent sliding down slopes by doing nothing
        if (p_params.movement_pass == MovementPass::GRAVITY || p_params.movement_pass == MovementPass::SNAP) {
            r_out.done = true;
            return;
        }
    } else {
        if (p_params.movement_pass == MovementPass::LATERAL) {
            // Attempt stair step
            StairSnapResult result;
            _try_snap_up_stair(p_params.desired_movement_direction, r_out.new_transform, result);

            if (result.snapped) {
                const Vector3 actual_travel = result.resulting_position - p_params.starting_trf.origin;
                const float actual_travel_length = actual_travel.length() - p_params.motion_margin;
				// This may sometimes be negative, this is intentional (for conservation of energy)
				// otherwise it looks like we are going up the stairs faster than should be possible
                const float actual_remainder = p_params.motion.length() - actual_travel_length;
                r_out.new_transform.origin = result.resulting_position;
                remainder = p_params.desired_movement_direction * actual_remainder;
                r_out.done = true;
                r_out.movement_snapped = true;
            } else {
                // Slide along a wall
                Vector3 lateral_normal = shape_cast_result->get_collision_normal();
                lateral_normal.y = 0.0f;
                lateral_normal.normalize();

                Vector3 lateral_desired_movement = p_params.desired_movement_direction;
                lateral_desired_movement.y = 0.0f;
                lateral_desired_movement.normalize();
                float wall_move_scale = 1.0f - lateral_normal.dot(-lateral_desired_movement);
                remainder = _plane_project(lateral_normal, remainder) * wall_move_scale;
            }
        }
    }
    r_out.remaining_velocity = remainder;
}
