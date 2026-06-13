#pragma once

#include "bind_macros.h"
#include "godot_cpp/classes/container.hpp"

using namespace godot;

class RadialContainer : public Container {
	GDCLASS(RadialContainer, Container);

	float animation_progress = 1.0f;

public:
	void set_animation_progress(float p_animation_progress);
	float get_animation_progress() const;
	static void _bind_methods();
	void _notification(int p_what);
	void _sort_children();
};
