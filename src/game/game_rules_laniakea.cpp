#include "game_rules_laniakea.h"

#include "game/weapon_counter_shield.h"
#include "game/weapon_gravitygun.h"
#include "game/weapon_rifle_test.h"
#include "gdextension_interface.h"
#include "godot_cpp/classes/resource_loader.hpp"
#include "godot_cpp/classes/texture2d.hpp"

CVar LaniakeaGameRules::rifle_max_ammo_cvar = CVar::create_variable("sk_max_rifle", GDEXTENSION_VARIANT_TYPE_INT, 120, "Maximum rifle ammo", PROPERTY_HINT_NONE, "");
CVar LaniakeaGameRules::rifle_player_damage_cvar = CVar::create_variable("sk_plr_dmg_rifle", GDEXTENSION_VARIANT_TYPE_INT, 5, "Rifle damage to player", PROPERTY_HINT_NONE, "");
CVar LaniakeaGameRules::rifle_npc_damage_cvar = CVar::create_variable("sk_npc_dmg_rifle", GDEXTENSION_VARIANT_TYPE_INT, 5, "Rifle damage to NPCs", PROPERTY_HINT_NONE, "");

void LaniakeaGameRules::initialize() {
	register_ammo_type(RIFLE_AMMO_TYPE, "rifle", "sk_max_rifle", "sk_plr_dmg_rifle", "sk_npc_dmg_rifle");
	register_item_type("weapon_rifle_test_item", { .localization_name = "#LN_Rifle", .localization_description = "#LN_Rifle_Description", .icon = ResourceLoader::get_singleton()->load("uid://dcjxl207338c0") });
	register_item_type("weapon_gravitygun_item", { .localization_name = "#LN_GravityGun", .localization_description = "#LN_GravityGun_Description", .icon = ResourceLoader::get_singleton()->load("uid://bn1exrtn22bju") });
	register_item_type("weapon_counter_shield_item", { .localization_name = "#LN_CounterShield", .localization_description = "#LN_CounterShield_Description", .icon = ResourceLoader::get_singleton()->load("uid://bn1exrtn22bju") });
	register_weapon<WeaponGravityGun>("weapon_gravitygun");
	register_weapon<WeaponRifleTest>("weapon_rifle_test");
	register_weapon<WeaponCounterShield>("weapon_counter_shield");
	GameRules::initialize();
}
