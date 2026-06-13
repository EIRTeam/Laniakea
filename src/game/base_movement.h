#pragma once

#include "game/movement_shared.h"
#include "godot_cpp/classes/node3d.hpp"
#include "godot_cpp/classes/physics_test_motion_result3d.hpp"
#include "godot_cpp/classes/shape3d.hpp"
#include "godot_cpp/variant/transform3d.hpp"
#include "movement_settings.h"
#include "springs.h"

class BaseMovement {
public:
    enum MovementPass {
        LATERAL,
        GRAVITY,
        SNAP
    };
private:
    struct MovementIterParams {
        const Transform3D &starting_trf;
        const Vector3 &motion;
        const Vector3 &desired_movement_direction;
        const MovementPass movement_pass;
        const float motion_margin = 0.001f;
    };

    struct MovementIterResult {
        bool done = false;
        bool hit_something = false;
        bool movement_snapped = false;
        Vector3 remaining_velocity;
        Transform3D new_transform;
        Ref<PhysicsTestMotionResult3D> kinematic_collision_result;
    };

    struct MovementPassResult {
        bool hit_something = false;
        bool movement_snapped = false;
    };

    Ref<MovementSettings> movement_settings;
    Node3D *owner_node = nullptr;

    Ref<Shape3D> body_shapes_per_stance[Movement::STANCE_MAX];

    Movement::MovementStance current_stance = Movement::STANDING;

    RID body;

    struct StairSnapResult {
        bool snapped = false;
        Vector3 resulting_position;
    };

    float vertical_velocity = 0.0f;
    Vector3 desired_velocity;
    bool grounded = false;
    Movement::MovementSpeed desired_movement_speed = Movement::MovementSpeed::IDLING;
    Vector3 effective_velocity;
    Vector2 input_vector;

    Springs::PositionSpringCritical desired_velocity_spring;

    void _movement_iter(const MovementIterParams &p_params, MovementIterResult &r_out) const;
    bool _test_move(const Transform3D &p_trf, const Vector3 &p_motion, Ref<PhysicsTestMotionResult3D> &r_result, float p_margin = 0.001f) const;
    void _try_snap_up_stair(const Vector3 &p_movement_dir, const Transform3D &p_starting_trf, StairSnapResult &r_result) const;
    void _do_movement_pass(const MovementPass p_pass, float p_delta, MovementPassResult &r_out);
public:
    void set_input_vector(const Vector2 &p_input_vector);
    void set_desired_movement_speed(Movement::MovementSpeed p_movement_speed);
    Vector3 get_desired_velocity() const;
    void initialize(Ref<MovementSettings> p_movement_settings, Node3D *p_owner);
    void update(float p_delta);
    void _handle_collision(const Vector3 &p_velocity, const Ref<PhysicsTestMotionResult3D> &p_collision_result);
    Vector3 get_smoothed_desired_velocity() const;
    Vector3 get_effective_velocity() const;
    Movement::MovementStance get_current_stance() const;
    void set_enabled(bool p_enabled);
    void add_collision_exception(RID p_body);
    void remove_collision_exception(RID p_body);
    ~BaseMovement();
};
