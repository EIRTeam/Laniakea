#include "weapon_data.h"

#include "bind_macros.h"
#include "godot_cpp/core/class_db.hpp"

void WeaponData::_bind_methods() {
	MAKE_BIND_STRING_NAME(WeaponData, id);
}
