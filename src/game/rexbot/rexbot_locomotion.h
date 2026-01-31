#include "game/rexbot/rexbot_npc_base.h"

class RexbotConfiguration;
class MovementSettings;

class RexbotLocomotion {
    RexbotNPCBase *npc;
    Ref<RexbotConfiguration> config;
    Ref<MovementSettings> movement_config;
public:
    RexbotLocomotion(RexbotNPCBase *p_npc, Ref<RexbotConfiguration> p_config);
};