#pragma once

#include "godot_cpp/classes/marker3d.hpp"

using namespace godot;

class PlayerSpawnPoint : public Marker3D {
	GDCLASS(PlayerSpawnPoint, Marker3D);
	PlayerSpawnPoint();
};
