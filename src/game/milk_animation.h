#pragma once

#include "biped_animation_base.h"
#include "console/cvar.h"
#include "springs.h"

class MilkAnimation : public BipedAnimationBase {
	GDCLASS(MilkAnimation, BipedAnimationBase);

	static CVar milk_hanging_offset_max_cvar;
	static CVar milk_hanging_spring_halflife_cvar;

	static void _bind_methods();

	Springs::OffsetSpringCritical hip_offset_spring;

	Vector3 carrier_angular_velocity;

	Vector3 hip_offset;

	Vector3 prev_model_pos;

public:
	void set_carrier_angular_velocity(Vector3 p_carrier_angular_velocity);
	virtual void update(Movement::MovementSpeed p_desired_movement_speed, float p_delta) override;
};
