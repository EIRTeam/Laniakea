#pragma once

#include "bind_macros.h"
#include "godot_cpp/classes/resource.hpp"

using namespace godot;

class RexbotConfiguration : public Resource {
    GDCLASS(RexbotConfiguration, Resource);

    float vision_base_fov = Math::deg_to_rad(90.0f);
    float vision_base_fov_half_cos = Math::cos(0.5 * Math::deg_to_rad(90.0f));
    float vision_reaction_time = 0.5f;

    static void _bind_methods();
public:
    void set_vision_base_fov(float p_vision_base_fov);
	float get_vision_base_fov() const;
	float get_vision_base_fov_half_cos() const;
    MAKE_SETTER_GETTER_VALUE(float, vision_reaction_time, vision_reaction_time);
};