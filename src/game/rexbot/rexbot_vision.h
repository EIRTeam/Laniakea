#pragma once
#include "console/cvar.h"
#include "game/rexbot/rexbot_npc_base.h"

class RexbotConfiguration;

class RexbotVision {
    static CVar draw_viewcone_cvar;
    RexbotNPCBase *npc;
    Ref<RexbotConfiguration> config;
    struct KnownActor {
        ObjectID actor_object;
        double became_visible_time = 0.0f;
        bool visible = false;
        bool aware = false;
    };

    LocalVector<KnownActor> known_actors;
public:
    RexbotVision(RexbotNPCBase *p_npc, Ref<RexbotConfiguration> p_config);
    bool is_in_viewcone(const Vector3 &p_position) const;
    void update();
    int get_primary_threat_idx() const;
    BaseCharacter *get_known_actor_character(int p_idx) const;


    String get_debug_string() const;
};