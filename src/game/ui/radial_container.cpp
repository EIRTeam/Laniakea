#include "radial_container.h"

#include "godot_cpp/core/error_macros.hpp"

void RadialContainer::set_animation_progress(float p_animation_progress) {
	animation_progress = CLAMP(p_animation_progress, 0.0f, 1.0f);
	queue_sort();
}

float RadialContainer::get_animation_progress() const {
	return animation_progress;
}

void RadialContainer::_bind_methods() {
	MAKE_BIND_FLOAT(RadialContainer, animation_progress);
}

void RadialContainer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_SORT_CHILDREN: {
			_sort_children();
		} break;
	}
}

void RadialContainer::_sort_children() {
	const float rads_per_child = Math::deg_to_rad(360.0f) / static_cast<float>(get_child_count());

	const Vector2 our_size = get_size();
	const Vector2 our_size_half = get_size() * 0.5f;
	const float our_size_shortest_half = our_size_half[our_size_half.min_axis_index()];

	for (int i = 0; i < get_child_count(); i++) {
		Control *child = Object::cast_to<Control>(get_child(i));
		ERR_BREAK_MSG(!child, "All children must inherit from Control");
		const Vector2 child_size = child->get_combined_minimum_size();
		child->set_size(child_size);
		Vector2 target_position = Vector2(our_size_shortest_half - child_size[child_size.max_axis_index()] * 0.5f, 0.0f).rotated(rads_per_child * i);
		target_position *= animation_progress;
		target_position += our_size_half;
		target_position -= child_size * 0.5f;
		//target_position.y -= child_size.y * 0.5f;
		child->set_position(target_position);
	}
}
