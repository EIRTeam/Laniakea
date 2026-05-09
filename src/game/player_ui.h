#pragma once

#include "godot_cpp/classes/control.hpp"
#include "base_character.h"
#include "godot_cpp/classes/label.hpp"

using namespace godot;

namespace godot {
class Camera3D;
}

class PlayerCharacter;
class ItemSelectorUI;
class WeaponFirearmInstance;

class PlayerUI : public Control {
    GDCLASS(PlayerUI, Control);

    Control *crosshairs_container = nullptr;
    Control *occlusion_crosshairs_container = nullptr;
    ItemSelectorUI *current_selector_ui = nullptr;
    Label *ammo_label = nullptr;
public:
    void notify_equip_item_requested(StringName p_item_name, int p_slot);
    void notify_unequip_item_requested(int p_slot);
    void notify_firearm_equipped(const Ref<WeaponFirearmInstance> &p_weapon, int p_slot, int p_chambered_ammo, int p_ammo_pool_total);
    void notify_firearm_ammo_spent(int p_slot, int p_chambered_ammo, int p_ammo_pool_total);
    void notify_firearm_ammo_acquired(int p_slot, int p_chambered_ammo, int p_ammo_pool_total);
    void notify_firearm_reloaded(int p_slot, int p_chambered_ammo, int p_ammo_pool_total);
    static void _bind_methods();
    virtual void _ready() override;
    void _on_equip_weapon_requested(StringName p_weapon, BaseCharacter::WeaponSlot p_slot);
    void update(PlayerCharacter *p_player, float p_delta);
    void update_crosshair(int p_slot, Camera3D *p_camera, bool p_show_crosshair, bool p_occluded, Vector3 occlusion_position);
    void show_weapon_selector(PlayerCharacter *p_character);
};