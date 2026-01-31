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

    RexbotBrain *brain = nullptr;
    RexbotBehaviour *behaviour = nullptr;
    Ref<RexbotConfiguration> configuration;

    MAKE_SETTER_GETTER_VALUE(Ref<RexbotConfiguration>, configuration, configuration);

    static void _bind_methods();

    virtual RexbotBehaviour *create_starting_behaviour() = 0;
public:
    virtual void _ready() override;
    virtual void _physics_process(double p_delta) override;
    RexbotBrain *get_brain() const;

    virtual ~RexbotNPCBase();
};