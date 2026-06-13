#pragma once

#include "godot_cpp/classes/a_star_grid2d.hpp"
using namespace godot;
class CoarseAStar {
	Ref<AStarGrid2D> astar;
	CoarseAStar() {
		astar.instantiate();
	}

	virtual float get_cost(const Vector2 &p_from, const Vector2 &p_to) const = 0;
};
