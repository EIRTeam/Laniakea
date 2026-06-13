#pragma once

#include "bind_macros.h"
#include "godot_cpp/classes/resource.hpp"
#include "godot_cpp/core/binder_common.hpp"

using namespace godot;

class WeaponData : public Resource {
	GDCLASS(WeaponData, Resource);

private:
	StringName id;
	bool can_be_aimed_down_sights = false;

public:
	static void _bind_methods();
	MAKE_SETTER_GETTER_VALUE(bool, can_be_aimed_down_sights, can_be_aimed_down_sights);
	MAKE_SETTER_GETTER_VALUE(StringName, id, id);
};
