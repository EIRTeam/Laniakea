#pragma once

#include "godot_cpp/classes/image.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/variant/vector2.hpp"
#include "godot_cpp/variant/vector2i.hpp"

using namespace godot;

float bilinearly_sample_image_single_channel(Ref<Image> p_image, int p_channel_idx, const Vector2 &p_uv);
