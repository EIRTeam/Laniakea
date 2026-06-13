#include "weapon_instance.h"

#include "game/game_rules_laniakea.h"
#include "game/main_loop.h"

void WeaponInstanceBase::_bind_methods() {
}

bool WeaponInstanceBase::uses_occluded_crosshair() const {
	return false;
}

StringName WeaponInstanceBase::get_weapon_name() const {
	return LaniakeaMainLoop::get_singleton()->get_game_rules()->weapon_name_from_item_name(get_item_name());
}
