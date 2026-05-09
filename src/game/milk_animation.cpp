#include "milk_animation.h"
#include "biped_animation_base.h"
#include "character_model.h"

CVar MilkAnimation::milk_hanging_offset_max_cvar = CVar::create_variable("milk.hanging_offset_max", GDEXTENSION_VARIANT_TYPE_FLOAT, 0.1f, "Maximum amount of deviation milk can have in her velocity when hanging off the back", PROPERTY_HINT_NONE, "");
CVar MilkAnimation::milk_hanging_spring_halflife_cvar = CVar::create_variable("milk.milk_hanging_spring_halflife", GDEXTENSION_VARIANT_TYPE_FLOAT, 0.1f, "Halflife for milk's hanging spring", PROPERTY_HINT_NONE, "");

void MilkAnimation::set_carrier_angular_velocity(Vector3 p_carrier_angular_velocity) {
    carrier_angular_velocity = p_carrier_angular_velocity;
}

void MilkAnimation::update(Movement::MovementSpeed p_desired_movement_speed, float p_delta) {
    BipedAnimationBase::update(p_desired_movement_speed, p_delta);

    // Calculate our linear velocity

    const Vector3 linear_change = model->get_global_position() - prev_model_pos;
    prev_model_pos = model->get_global_position();

    hip_offset_spring.initialize(milk_hanging_spring_halflife_cvar.get_float());

    hip_offset -= linear_change;

    hip_offset_spring.reset(hip_offset, false);
    hip_offset_spring.update(Vector3(), p_delta);
    hip_offset = hip_offset_spring.get();
    hip_offset = hip_offset.clampf(-milk_hanging_offset_max_cvar.get_float(), milk_hanging_offset_max_cvar.get_float());

    ERR_FAIL_COND(model->get_milk_hip_target() == nullptr);

    Vector3 hip_offset_in_hip_space = model->get_milk_hip_target()->get_global_basis().xform_inv(hip_offset);
    model->get_milk_hip_target()->set_position(hip_offset_in_hip_space);
}

