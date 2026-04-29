#pragma once

#include "bind_macros.h"
#include "godot_cpp/classes/ref_counted.hpp"

using namespace godot;

class CharacterSettings : public RefCounted {
    GDCLASS(CharacterSettings, RefCounted);

    int max_health = 100;

    
    static void _bind_methods();

public:
    MAKE_SETTER_GETTER_VALUE(int32_t, max_health, max_health);
};