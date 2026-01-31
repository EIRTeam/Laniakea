#include "rexbot_locomotion.h"
#include "rexbot_configuration.h"
#include "game/movement_settings.h"

RexbotLocomotion::RexbotLocomotion(RexbotNPCBase *p_npc, Ref<RexbotConfiguration> p_config) {
    npc = p_npc;
    config = p_config;
    movement_config = p_npc->get_movement_settings();
    DEV_ASSERT(movement_config.is_valid());
}
