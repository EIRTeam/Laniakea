#include "weapon_rifle_test.h"
#include "game/main_loop.h"
#include "game/weapon_rifle_test.h"
#include "godot_cpp/classes/packed_scene.hpp"
#include "godot_cpp/classes/resource_loader.hpp"
#include "weapon_model.h"
#include "game/game_rules_laniakea.h"

WeaponModel *WeaponRifleTest::instantiate_visuals() const {
    Ref<PackedScene> visuals = ResourceLoader::get_singleton()->load("res://models/weapons/w_rifle.tscn");
    WeaponModel *visual_instance = Object::cast_to<WeaponModel>(visuals->instantiate());
    return visual_instance;
}

StringName WeaponRifleTest::get_item_name() const {
    static StringName s = "weapon_rifle_test_item";
    return s;
}

int WeaponRifleTest::get_ammo_type() const {
    return LaniakeaGameRules::RIFLE_AMMO_TYPE;
}
