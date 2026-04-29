#pragma once

#include "bind_macros.h"
#include "console/cvar.h"
#include "game/base_character.h"
#include "game/rexbot/rexbot_configuration.h"

class RexbotBehaviour;
class RexbotBrain;

class RexbotNPCBase : public BaseCharacter {
    GDCLASS(RexbotNPCBase, BaseCharacter);
    
    static CVar rexbot_debug_behaviour_cvar;
    static CVar rexbot_debug_vision_cvar;
    static CVar rexbot_debug_health_cvar;

    RexbotBrain *brain = nullptr;
    RexbotBehaviour *behaviour = nullptr;
    Ref<RexbotConfiguration> configuration;

    MAKE_SETTER_GETTER_VALUE(Ref<RexbotConfiguration>, configuration, configuration);

    static void _bind_methods();

    virtual RexbotBehaviour *create_starting_behaviour() = 0;

    struct NPCButtonInputState {
        BitField<InputActionState> state = 0;
        float time_left = 0.0f;
    };

    struct NPCInputState {
        std::array<NPCButtonInputState, INPUT_COMMAND_MAX> buttons;
        Vector2 movement_vector;
    };

    NPCInputState input_state;

    Vector3 look_direction;

public:
    virtual void _ready() override;
    virtual void _physics_process(double p_delta) override;
    RexbotBrain *get_brain() const;
    virtual Vector2 get_movement_vector() const override;
    virtual Vector2 get_movement_vector_transformed() const override;
    virtual BitField<InputActionState> get_action_state(InputCommand p_action) const override;
    virtual void get_aim_trajectory(int p_weapon_slot, Vector3 &r_origin, Vector3 &r_direction) override;
    virtual Vector3 get_look_direction() const override;
    void set_look_direction(const Vector3 &p_look_direction);

    void press_primary_fire(float p_duration = 0.0f);

    virtual ~RexbotNPCBase();
    friend RexbotBehaviour;
};