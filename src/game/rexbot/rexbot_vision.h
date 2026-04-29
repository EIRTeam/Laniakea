#pragma once
#include "console/cvar.h"
#include "game/ln_timer.h"
#include "game/rexbot/rexbot_npc_base.h"
#include "godot_cpp/classes/timer.hpp"
#include "springs.h"
#include <optional>

class RexbotConfiguration;

class RexbotVision {
    static CVar draw_viewcone_cvar;
    static CVar draw_lookat_dir_cvar;
    static CVar rexbot_lookat_debug_cvar;
    RexbotNPCBase *npc;
    Ref<RexbotConfiguration> config;
    struct KnownActor {
        ObjectID actor_object;
        double became_visible_time = 0.0f;
        bool visible = false;
        bool aware = false;
    };

    LocalVector<KnownActor> known_actors;
    Vector3 get_aim_direction() const;
    Springs::QuaternionSpringCritical aim_direction_spring;
public:
    enum class LookAtTargetPriority {
        LOW,
        MID,
        HIGH
    };
private:
    struct LookAtTarget {
        enum LookAtTargetType {
            POSITION,
            ACTOR
        };

        LookAtTargetType target_type = LookAtTargetType::POSITION;

        Vector3 tracked_position;
        BaseCharacter *actor;

        float duration = 0.0f;
        LookAtTargetPriority priority = LookAtTargetPriority::LOW;
        String reason;
    };

    bool is_aim_sighted_in = false;

    Timer *update_aim_direction_timer = nullptr;

    std::optional<LookAtTarget> look_at_target;
    bool _try_lookat(const LookAtTarget &p_target);
    void _update_aim_direction();
    LNTimer::LNIntervalTimer aim_interval_timer;
public:
    double get_entity_tracking_update_interval() const;
    RexbotVision(RexbotNPCBase *p_npc, Ref<RexbotConfiguration> p_config);
    bool is_in_viewcone(const Vector3 &p_position) const;
    void update(double p_delta);
    int get_primary_threat_idx() const;
    BaseCharacter *get_known_actor_character(int p_idx) const;
    void update_desired_look_direction(const Quaternion &p_aim_dir);
    
    bool aim_head_to_character(BaseCharacter *p_character, LookAtTargetPriority p_priority, float p_duration, String p_reason);
    bool aim_head_to_position(Vector3 p_position, LookAtTargetPriority p_priority, float p_duration, String p_reason);
    bool is_aiming_at_target() const;

    String get_debug_string() const;
};