#include "rexbot_configuration.h"
#include "bind_macros.h"
#include "godot_cpp/core/class_db.hpp"

void RexbotConfiguration::_bind_methods() {
	ADD_GROUP("Vision", "vision_");
    MAKE_BIND_FLOAT(RexbotConfiguration, vision_reaction_time);
    ClassDB::bind_method(D_METHOD("set_vision_base_fov", "vision_base_fov"), &RexbotConfiguration::set_vision_base_fov);
    ClassDB::bind_method(D_METHOD("get_vision_base_fov"), &RexbotConfiguration::get_vision_base_fov);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "vision_base_fov", PROPERTY_HINT_RANGE, "0,360,1,radians_as_degrees"), "set_vision_base_fov", "get_vision_base_fov");
}

void RexbotConfiguration::set_vision_base_fov(float p_vision_base_fov) {
    vision_base_fov = p_vision_base_fov;
    vision_base_fov_half_cos = Math::cos(0.5 * Math::deg_to_rad(90.0f));
}

float RexbotConfiguration::get_vision_base_fov() const {
    return vision_base_fov;
}

float RexbotConfiguration::get_vision_base_fov_half_cos() const {
    return vision_base_fov_half_cos;
}
