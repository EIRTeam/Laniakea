#include "rexbot_vision.h"
#include "debug/debug_constexpr.h"
#include "game/ln_timer.h"
#include "game/main_loop.h"
#include "gdextension_interface.h"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/math.hpp"
#include "rexbot_configuration.h"

CVar RexbotVision::draw_viewcone_cvar = CVar::create_variable("rb.draw_viewcones", GDEXTENSION_VARIANT_TYPE_BOOL, false, "Whether or not to draw viewcones");
CVar RexbotVision::draw_lookat_dir_cvar = CVar::create_variable("rb.draw_lookat_dir", GDEXTENSION_VARIANT_TYPE_BOOL, false, "Whether or not to draw the lookat dir");
CVar RexbotVision::rexbot_lookat_debug_cvar = CVar::create_variable("rb.debug_lookat", GDEXTENSION_VARIANT_TYPE_BOOL, false, "Whether or not to show messages when lookat changes");

Vector3 RexbotVision::get_aim_direction() const {
    if (!look_at_target.has_value()) {
        return npc->get_facing_direction();
    }
    return npc->get_model()->get_eye_position().direction_to(look_at_target->tracked_position);
}

bool RexbotVision::_try_lookat(const LookAtTarget &p_target)
{

    bool is_same = false;

    if (look_at_target.has_value()) {
        is_same = p_target.priority >= look_at_target->priority && p_target.target_type == look_at_target->target_type;
        if (is_same) {
            switch (p_target.target_type) {
                case LookAtTarget::POSITION: {
                    is_same = p_target.tracked_position.is_equal_approx(look_at_target->tracked_position);
                } break;
                case LookAtTarget::ACTOR: {
                    is_same = p_target.actor == look_at_target->actor;
                } break;
            }
            // Same thing, just update duration and priority
            look_at_target->duration = p_target.duration;
            look_at_target->priority = p_target.priority;
            return true;
        }

		if (look_at_target->priority >= p_target.priority) {
            if (rexbot_lookat_debug_cvar.get_bool()) {
                print_error(vformat("Rejected lookat (%s) with priority %d because we are already looking at something with a higher or equal priority of %d (%s)",
                    p_target.reason,
                    (int)p_target.priority,
                    (int)look_at_target->priority,
                    look_at_target->reason
                ));
            }

            return false;
        }
    }

    look_at_target = p_target;

    if (Debug::is_debug_enabled && rexbot_lookat_debug_cvar.get_bool()) {
        String target_name_string;
        switch (look_at_target->target_type) {
			case LookAtTarget::POSITION: {
                target_name_string = vformat("position %s", look_at_target->tracked_position);
            } break;
			case LookAtTarget::ACTOR: {
                target_name_string = vformat("actor %s", look_at_target->actor->get_path());
            } break;
		}

        print_line(vformat("Look At %s with priority %d (%s)", target_name_string, static_cast<int>(look_at_target->priority), look_at_target->reason));
    }

    return true;
}

double RexbotVision::get_entity_tracking_update_interval() const {
    return 0.1f;
}

RexbotVision::RexbotVision(RexbotNPCBase *p_npc, Ref<RexbotConfiguration> p_config) : aim_interval_timer(LNTimer::LNTimerUpdateMode::PHYSICS) {
    npc = p_npc;
    config = p_config;
}

bool RexbotVision::is_in_viewcone(const Vector3 &p_position) const {
    const Vector3 diff = p_position - npc->get_model()->get_eye_position();
    const float cos_diff = diff.dot(npc->get_look_direction());

    if (cos_diff < 0.0) {
        return false;
    }

    const float cos_half_pov = config->get_vision_base_fov_half_cos();
    return cos_diff * cos_diff > diff.length_squared() * cos_half_pov * cos_half_pov;
}

void RexbotVision::update(double p_delta) {
    // Check what characters we can see
    static const StringName character_group = "characters";
    TypedArray<Node> potentially_visible_characters = LaniakeaMainLoop::get_singleton()->get_nodes_in_group(character_group);

    LocalVector<BaseCharacter *>visible_characters;

    for (int i = 0; i < potentially_visible_characters.size(); i++) {
        BaseCharacter *character = Object::cast_to<BaseCharacter>(potentially_visible_characters[i].get_validated_object());
        if (!character || character == npc) {
            continue;
        }

        if (!is_in_viewcone(character->get_global_position())) {
            continue;
        }

        // TODO: Perform raycast to test actual visibility

        visible_characters.push_back(character);
    }

    // Update existing known entities
    const float vision_reaction_time = config->get_vision_reaction_time();
    for (KnownActor &known_actor : known_actors) {
        if (!known_actor.actor_object.is_valid()) {
            continue;
        }

        BaseCharacter *character = Object::cast_to<BaseCharacter>(ObjectDB::get_instance(known_actor.actor_object));
        bool was_visible = known_actor.visible;

        if (!was_visible && visible_characters.has(character)) {
            known_actor.visible = true;
            known_actor.became_visible_time = LaniakeaMainLoop::get_singleton()->get_physics_time();
        } else if (was_visible && !visible_characters.has(character)) {
            known_actor.visible = false;
            known_actor.aware = false;
            // TODO: Emit lost sight event
        } else if (was_visible && !known_actor.aware && visible_characters.has(character)) {
            if (LaniakeaMainLoop::get_singleton()->get_physics_time() - known_actor.became_visible_time >= vision_reaction_time) {
                known_actor.aware = true;
            }
        }
    }

    // Add newly sighted entities
    for (BaseCharacter *character : visible_characters) {
        const ObjectID instance_id = ObjectID(character->get_instance_id());

        int j;
        for (j = 0; j < known_actors.size(); j++) {
            if (known_actors[j].actor_object == instance_id) {
                break;
            }
        }

        if (j != known_actors.size()) {
            continue;
        }

        known_actors.push_back({
            .actor_object = instance_id,
            .became_visible_time = LaniakeaMainLoop::get_singleton()->get_physics_time(),
            .visible = true,
            .aware = false
        });
    }


    if (look_at_target.has_value()) {
        look_at_target->duration -= p_delta;
        if (look_at_target->duration <= 0.0f) {
            if (rexbot_lookat_debug_cvar.get_bool()) {
                print_line(vformat("Finishing lookat %s because the duration ran out", look_at_target->reason));
            }
            look_at_target = std::nullopt;
        } else if (look_at_target->target_type == LookAtTarget::LookAtTargetType::ACTOR && aim_interval_timer.is_greater_than(get_entity_tracking_update_interval())) {
            // When tracking NPCs we have a custom tracking interval
            look_at_target->tracked_position = look_at_target->actor->get_model()->get_eye_position();
            aim_interval_timer.reset();
        }
    }

    Vector3 desired_aim_direction = get_aim_direction();
    aim_direction_spring.update(Quaternion(Vector3(0.0f, 0.0f, -1.0f), desired_aim_direction), p_delta);
    npc->set_look_direction(aim_direction_spring.get_value().xform(Vector3(0.0, 0.0, -1.0f)).normalized());

    const float SIGHTED_IN_TOLERANCE = Math::deg_to_rad(2.0);
    is_aim_sighted_in = desired_aim_direction.angle_to(npc->get_look_direction()) < SIGHTED_IN_TOLERANCE;
    DebugOverlay::line(npc->get_global_position(), npc->get_global_position() + npc->get_look_direction(), Color(1.0, 0.0, 0.0));
    DebugOverlay::line(npc->get_model()->get_eye_position(), npc->get_model()->get_eye_position() + desired_aim_direction, Color(0.0, 1.0, 0.0));

    if constexpr (Debug::is_debug_enabled) {
        if (draw_viewcone_cvar.get_bool()) {
            const Vector3 eye_pos = npc->get_model()->get_eye_position();
            DebugOverlay::cone(eye_pos, eye_pos + npc->get_look_direction() * 10.0f, config->get_vision_base_fov() * 0.5f, Color(1.0, 0.0, 1.0));
            DebugOverlay::vert_arrow(eye_pos, eye_pos + npc->get_look_direction() * 1.0f, 0.25f, Color(1.0, 0.0, 1.0));
            DebugOverlay::vert_arrow(eye_pos, eye_pos + desired_aim_direction * 1.0f, 0.25f, Color(0.0, 0.0, 1.0));
        }
    }
}

int RexbotVision::get_primary_threat_idx() const {
    // TODO: Improve this to make more sense...
    for (int i = 0; i < known_actors.size(); i++) {
        if (known_actors[i].aware) {
            return i;
        }
    }

    return -1;
}

BaseCharacter *RexbotVision::get_known_actor_character(int p_idx) const {
    ERR_FAIL_INDEX_V(p_idx, known_actors.size(), nullptr);

    return Object::cast_to<BaseCharacter>(ObjectDB::get_instance(known_actors[p_idx].actor_object));
}

void RexbotVision::update_desired_look_direction(const Quaternion &p_aim_dir) {
    aim_direction_spring.reset(p_aim_dir, false);
}

bool RexbotVision::aim_head_to_character(BaseCharacter *p_character, LookAtTargetPriority p_priority, float p_duration, String p_reason) {
    return _try_lookat({
        .target_type = LookAtTarget::LookAtTargetType::ACTOR,
        .tracked_position = p_character->get_model()->get_eye_position(),
        .actor = p_character,
        .duration = MAX(0.1f, p_duration),
        .priority = p_priority,
        .reason = p_reason
    });
}

bool RexbotVision::aim_head_to_position(Vector3 p_position, LookAtTargetPriority p_priority, float p_duration, String p_reason) {
    return _try_lookat({
        .target_type = LookAtTarget::LookAtTargetType::POSITION,
        .tracked_position = p_position,
        .duration = MAX(0.1f, p_duration),
        .priority = p_priority,
        .reason = p_reason
    });
}

bool RexbotVision::is_aiming_at_target() const {
    return is_aim_sighted_in;
}

String RexbotVision::get_debug_string() const {
    int actors_aware_of = 0;
    int actors_in_view = 0;

    for (const KnownActor &k_a : known_actors) {
        if (k_a.aware) {
            actors_aware_of++;
        }
        if (k_a.visible) {
            actors_in_view++;
        }
    }

    return vformat("Known entities: %d\nEntities in view: %d\nEntities aware of %d", known_actors.size(), actors_in_view, actors_aware_of);
}
