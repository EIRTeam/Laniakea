#pragma once

#include "godot_cpp/classes/ref_counted.hpp"
#include "rexbot_vision.h"
#include "rexbot_locomotion.h"

class RexbotVision;
class RexbotLocomotion;
class RexbotConfiguration;

using namespace godot;

class RexbotBrain {
    RexbotVision vision;
    RexbotLocomotion locomotion;

    RexbotNPCBase *actor = nullptr;
public:
    void update(double p_delta);
    RexbotVision *get_vision() const;
    RexbotLocomotion *get_locomotion() const;
    RexbotBrain(RexbotNPCBase *p_actor, Ref<RexbotConfiguration> p_config);
};