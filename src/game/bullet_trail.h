#pragma once

#include "godot_cpp/classes/mesh_instance3d.hpp"

using namespace godot;

namespace godot {
class ShaderMaterial;
}

class BulletTrail : public MeshInstance3D {
	GDCLASS(BulletTrail, MeshInstance3D);

	Vector3 origin;
	Vector3 origin_to_target_dir;
	float distance = 0.0f;
	float velocity = 0.0f;

	float time = 0.0f;

public:
	void _process(double p_delta) override;
	virtual void _ready() override;
	static void _bind_methods();
	void initialize(Vector3 p_origin, Vector3 p_target, float p_velocity);
};
