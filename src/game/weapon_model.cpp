#include "weapon_model.h"

#include "bind_macros.h"
void WeaponModel::_bind_methods() {
	MAKE_BIND_NODE(WeaponModel, muzzle_location, Node3D);
}
