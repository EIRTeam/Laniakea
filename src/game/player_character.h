#pragma once

#include "base_character.h"
#include "game/biped_animation_base.h"
#include "game/character_model.h"
#include "game/movement_settings.h"
#include "game/movement_shared.h"
#include "game/player_camera.h"
#include "game/player_ui.h"

using namespace godot;

class PlayerCharacter : public BaseCharacter {
    GDCLASS(PlayerCharacter, BaseCharacter);
    PlayerCamera *camera = nullptr;
    Node3D *camera_offset_target = nullptr;
    Node3D *firing_position_node = nullptr;
    PlayerUI *player_ui = nullptr;
    
    struct SlotAimOcclusionInformation {
        Node3D *target_position_interp_node = nullptr;
        bool is_target_position_occluded = false;
        Vector3 aim_target_direction;
    };
    std::array<SlotAimOcclusionInformation, WEAPON_SLOT_MAX> per_slot_aim_occlusion_info;
    TypedArray<RID> occlusion_exceptions;

    BipedAnimationBase *player_animation = nullptr;

    struct PlayerCharacterInputState {
        Vector2 movement_input;
        std::array<int, INPUT_COMMAND_MAX> button_states;
    };

    PlayerCharacterInputState input_state;
public:
    Ref<MovementSettings> movement_settings;
    static CVar player_camera_horizontal_deadzone_radius;
    static CVar player_camera_distance_aim;
    static CVar player_camera_distance;
    MAKE_SETTER_GETTER_VALUE(Node3D *, camera_offset_target, camera_offset_target);
    MAKE_SETTER_GETTER_VALUE(PlayerUI *, player_ui, player_ui);
    static void _bind_methods();
    void _movement_physics_process(float p_delta);
    virtual void _ui_process(float p_delta);
    BitField<InputActionState> _get_action_state(const StringName p_state) const;
    virtual void _camera_process(float p_delta);
    virtual void _ready() override;
    virtual void _process(double p_delta) override;
    virtual void _physics_process(double p_delta) override;
    void get_camera_aim_trajectory(Vector3 &r_origin, Vector3 &r_direction) const;
    virtual void get_aim_trajectory(int p_weapon_slot, Vector3 &r_origin, Vector3 &r_direction) override;
    void add_camera_kick(float p_max_vertical_kick_angle, float p_fire_duration_time, float p_slide_limit_time);
    virtual Ref<MovementSettings> get_movement_settings() const override;
    Vector3 get_slot_target_position(int p_slot) const;

    Transform3D get_milk_attachment_transform() const;
    void add_aim_occlusion_exception(RID p_exception);
    void remove_occlusion_exception(RID p_exception);

    Vector<StringName> get_available_weapon_items(WeaponSlot p_slot) const;
    virtual CharacterAnimationBase *create_animation() const override;

    virtual Vector2 get_movement_vector() const override;
    virtual Vector2 get_movement_vector_transformed() const override;
    virtual BitField<InputActionState> get_action_state(InputCommand p_command) const override;

    PlayerCharacter();
};