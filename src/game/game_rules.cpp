#include "game_rules.h"
#include "console/console_system.h"
#include "godot_cpp/classes/texture2d.hpp"
#include "godot_cpp/core/class_db.hpp"

void GameRules::register_ammo_type(int p_id, const StringName p_name, const StringName p_ammo_max_cvar, const StringName p_player_damage_cvar, const StringName p_npc_damage_cvar) {
    
    ConsoleSystem *cs = ConsoleSystem::get_singleton();

    DEV_ASSERT(cs->get_cvar(p_ammo_max_cvar) != nullptr);
    DEV_ASSERT(cs->get_cvar(p_player_damage_cvar) != nullptr);
    DEV_ASSERT(cs->get_cvar(p_npc_damage_cvar) != nullptr);
    DEV_ASSERT(!ammo_types.has(p_id));

    AmmoTypeInfo *type_info = memnew(AmmoTypeInfo);

    ammo_types.insert(p_id, {
        .ammo_name = p_name,
        .ammo_max_cvar = cs->get_cvar(p_ammo_max_cvar),
        .ammo_player_damage_cvar = cs->get_cvar(p_player_damage_cvar),
        .ammo_npc_damage_cvar = cs->get_cvar(p_npc_damage_cvar)
    });
}

void GameRules::initialize() {
}

void GameRules::register_item_type(StringName p_item_id, const RegisterItemTypeParams &p_params) {
    DEV_ASSERT(!item_types.has(p_item_id));
    item_types.insert(p_item_id, {
        .localization_name = p_params.localization_name,
        .localization_description = p_params.localization_description,
        .icon = p_params.icon
    });
}

Ref<Texture2D> GameRules::item_get_icon(StringName p_item_name) const {
    ERR_FAIL_COND_V_MSG(!item_types.has(p_item_name), Ref<Texture2D>(), vformat("Item %s not found!", p_item_name));
    return item_types[p_item_name].icon;
}

StringName GameRules::item_get_localization_name(StringName p_item_name) const {
    ERR_FAIL_COND_V_MSG(!item_types.has(p_item_name), "", vformat("Item %s not found!", p_item_name));
    return item_types[p_item_name].localization_name;
}

StringName GameRules::item_get_localization_description(StringName p_item_name) const {
    ERR_FAIL_COND_V_MSG(!item_types.has(p_item_name), "", vformat("Item %s not found!", p_item_name));
    return item_types[p_item_name].localization_description;
}

Ref<WeaponInstanceBase> GameRules::weapon_from_item_name(StringName p_weapon_item_name) const {
    ERR_FAIL_COND_V_MSG(!item_types.has(p_weapon_item_name), nullptr, vformat("Item %s not found!", p_weapon_item_name));
    ERR_FAIL_COND_V_MSG(!item_types[p_weapon_item_name].flags.has_flag(IS_WEAPON), nullptr, vformat("Item %s was not a weapon!", p_weapon_item_name));
    
    const StringName &weapon_name = item_types[p_weapon_item_name].weapon_name;
    ERR_FAIL_COND_V_MSG(!weapon_types.has(weapon_name), nullptr, vformat("Weapon %s not found!", weapon_name));
    
    Ref<WeaponInstanceBase> weapon = ClassDB::instantiate(weapon_types[weapon_name].class_name);
    ERR_FAIL_COND_V_MSG(weapon.is_null(), nullptr, vformat("Failed to instantiate weapon %s's class %s!", weapon_name, weapon_types[weapon_name].class_name));

    return weapon;
}

StringName GameRules::weapon_name_from_item_name(StringName p_weapon_item_name) const {
    ERR_FAIL_COND_V_MSG(!item_types.has(p_weapon_item_name), "", vformat("Item %s not found!", p_weapon_item_name));
    ERR_FAIL_COND_V_MSG(!item_types[p_weapon_item_name].flags.has_flag(IS_WEAPON), "", vformat("Item %s was not a weapon!", p_weapon_item_name));
    
    const StringName &weapon_name = item_types[p_weapon_item_name].weapon_name;
    return weapon_name;
}

int GameRules::get_ammo_type_damage(int p_ammo_type, bool p_is_npc) const {
    auto it = ammo_types.find(p_ammo_type);
    ERR_FAIL_COND_V(it == ammo_types.end(), 0.0f);
    if (p_is_npc) {
        it->value.ammo_npc_damage_cvar->get_int();
    }

    return it->value.ammo_player_damage_cvar->get_int();
}
