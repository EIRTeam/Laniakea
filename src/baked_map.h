#pragma once

#include "godot_cpp/classes/image.hpp"
#include "godot_cpp/templates/vector.hpp"

using namespace godot;

class BakedMap {
	Vector<float> data;

	Vector2i dimensions;

public:
	void from_image(Ref<Image> p_image);
	float sample_point(Vector2i p_point) const;
	float bilinearly_sample(Vector2 p_uv) const;
};
