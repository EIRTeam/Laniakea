#pragma once

#include "godot_cpp/classes/object.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/static_body3d.hpp"
#include "movement_shared.h"

using namespace godot;

class CharacterModel;
class MovementSettings;
class BaseCharacter;

class CharacterAnimationBase : public Object {
    GDCLASS(CharacterAnimationBase, Object);

public:
    static void _bind_methods();
    virtual void initialize(Ref<MovementSettings> p_movement_settings, CharacterModel *p_model) = 0;
    virtual void physics_update(BaseCharacter *p_movement, float p_delta) = 0;
    virtual void update(Movement::MovementSpeed p_desired_movement_speed, float p_delta) = 0;
};