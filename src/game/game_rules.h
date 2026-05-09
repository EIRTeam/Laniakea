#pragma once

#include "console/cvar.h"
#include "game/weapon_instance.h"
#include "godot_cpp/classes/object.hpp"
#include "godot_cpp/templates/hash_map.hpp"

using namespace godot;

namespace godot {
    class Texture2D;
}

class GameRules : public Object {
    GDCLASS(GameRules, Object);

    struct AmmoTypeInfo {
        StringName ammo_name;
        CVar *ammo_max_cvar;
        CVar *ammo_player_damage_cvar;
        CVar *ammo_npc_damage_cvar;
    };

    enum ItemFlags {
        IS_WEAPON = 1
    };

    struct ItemTypeInfo {
        StringName localization_name;
        StringName localization_description;
        BitField<ItemFlags> flags = 0;
        StringName weapon_name;
        Ref<Texture2D> icon;
    };
    
    HashMap<int, AmmoTypeInfo> ammo_types;
    HashMap<StringName, ItemTypeInfo> item_types;

    struct WeaponTypeInfo {
        StringName name;
        StringName class_name;
        StringName item_name;
    };
    HashMap<StringName, WeaponTypeInfo> weapon_types;
    static void _bind_methods() {}
public:
    void register_ammo_type(int p_id, const StringName p_name, const StringName p_ammo_max_cvar, const StringName p_player_damage_cvar, const StringName p_npc_damage_cvar);
    virtual void initialize();

    struct RegisterItemTypeParams {
        StringName localization_name;
        StringName localization_description;
        Ref<Texture2D> icon;
    };
    void register_item_type(StringName p_item_id, const RegisterItemTypeParams &p_params);
    bool has_item(StringName p_item_name) const {
        return item_types.has(p_item_name);
    }

    template<typename WeaponT>
    void register_weapon(StringName p_weapon_name) {
        static_assert(!TypeInherits<WeaponT, WeaponInstanceBase>::value, "Class must inherit from WeaponBase!");
        Ref<WeaponT> t;
        t.instantiate();
        weapon_types.insert(p_weapon_name, {
            .name = p_weapon_name,
            .class_name = WeaponT::get_class_static(),
            .item_name = t->get_item_name()
        });


        DEV_ASSERT(has_item(t->get_item_name()));

        ItemTypeInfo &item_type = item_types[t->get_item_name()];
        item_type.flags.set_flag(ItemFlags::IS_WEAPON);
        item_type.weapon_name = p_weapon_name;
        print_line("Register weapon! ", p_weapon_name);
    }

    Ref<Texture2D> item_get_icon(StringName p_item_name) const;
    StringName item_get_localization_name(StringName p_item_name) const;
    StringName item_get_localization_description(StringName p_item_name) const;
    Ref<WeaponInstanceBase> weapon_from_item_name(StringName p_weapon_item_name) const;
    StringName weapon_name_from_item_name(StringName p_weapon_item_name) const;
    int get_ammo_type_damage(int p_ammo_type, bool p_is_npc = false) const;
};