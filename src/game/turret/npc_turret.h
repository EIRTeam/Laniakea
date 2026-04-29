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
    virtual CharacterAnimationBase *create_animation() const override;
};