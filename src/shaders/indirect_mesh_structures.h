#pragma once

#include <cstdint>

using mat4 = float[16];
using vec4 = float[4];

#pragma pack(push, 1)
#include "indirect_mesh_structures_inc.glsl"

struct DCullData {
	float P00; // offset 0 (4 bytes)
	float P11; // offset 4 (4 bytes)
	float znear; // offset 8 (4 bytes)
	float zfar; // offset 12 (4 bytes)
	float frustum_planes[4]; // offset 16 (16 bytes)
	vec4 camera_position; // offset 32 (16 bytes)
	mat4 view; // offset 48 (64 bytes)
	int shadow_pass; // offset 116 (4 bytes)
};
#pragma pack(pop)
