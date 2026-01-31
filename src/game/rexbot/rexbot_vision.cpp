#include "rexbot_vision.h"
#include "debug/debug_constexpr.h"
#include "game/main_loop.h"
#include "gdextension_interface.h"
#include "godot_cpp/core/error_macros.hpp"
#include "rexbot_configuration.h"

CVar RexbotVision::draw_viewcone_cvar = CVar::create_variable("rb.draw_viewcones", GDEXTENSION_VARIANT_TYPE_BOOL, false, "Whether or not to draw viewcones");

RexbotVision::RexbotVision(RexbotNPCBase *p_npc, Ref<RexbotConfiguration> p_config) {
    npc = p_npc;
    config = p_config;
}

bool RexbotVision::is_in_viewcone(const Vector3 &p_position) const {
    const Vector3 diff = p_position - npc->get_global_position();
    const float cos_diff = diff.dot(npc->get_facing_direction());

    if (cos_diff < 0.0) {
        return false;
    }

    const float cos_half_pov = config->get_vision_base_fov_half_cos();
    return cos_diff * cos_diff > diff.length_squared() * cos_half_pov * cos_half_pov;
}

void RexbotVision::update() {
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

    if constexpr (Debug::is_debug_enabled) {
        if (draw_viewcone_cvar.get_bool()) {
            DebugOverlay::cone(npc->get_global_position(), npc->get_global_position() + npc->get_facing_direction() * 10.0f, config->get_vision_base_fov() * 0.5f, Color(1.0, 0.0, 1.0));
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
