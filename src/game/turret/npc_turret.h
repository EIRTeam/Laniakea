#pragma once

#include "game/biped_animation_base.h"
#include "game/character_animation_base.h"
#include "game/rexbot/rexbot_behaviour.h"
#include "game/rexbot/rexbot_npc_base.h"

class NPCTurret : public RexbotNPCBase {
	GDCLASS(NPCTurret, RexbotNPCBase);
	BipedAnimationBase *biped_anim = nullptr;
	static void _bind_methods();

public:
	virtual void _ready() override;
	virtual RexbotBehaviour *create_starting_behaviour() override;
	virtual BipedAnimationBase *create_animation() const override;
	virtual Ref<MovementSettings> get_movement_settings() const override;
};
