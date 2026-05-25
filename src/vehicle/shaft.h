#pragma once

#include "godot_cpp/classes/ref_counted.hpp"
#include "vehicle/vehicle_drivetrain_config.h"

using namespace godot;

struct VehicleInputState {
    float steer = 0.0f;
    float throttle = 0.0f;
    float clutch = 0.0f;
    float brake = 0.0f;
    int gear = 0;
};

class LNVehicleShaft : public RefCounted {
    GDCLASS(LNVehicleShaft, RefCounted);

public:
    struct UpstreamData {
        float inertia = 0.0f;
        float angular_velocity = 0.0f;
        float net_reaction_torque = 0.0f;
    };

    struct DownstreamData {
        float torque = 0.0f;             // Torque being pushed from the power source
        float reflected_inertia = 0.0f;  // The total inertia of everything "above" this node
    };
private:
    LNVehicleShaft *parent = nullptr;
    LocalVector<LNVehicleShaft*> children;
    StringName name;
protected:
    Ref<LNVehicleDrivetrainSettings> drivetrain_settings;
public:
    virtual bool has_input() const = 0;
    LNVehicleShaft *get_child(int p_child_idx) const {
        ERR_FAIL_INDEX_V(p_child_idx, get_output_count(), nullptr);
        return children[p_child_idx];
    };
    
    virtual void initialize() {
        children.resize(get_output_count());
        for (LNVehicleShaft*& shaft : children) {
            shaft = nullptr;
        }
    }
    virtual void apply_downstream(const DownstreamData &p_data) {
        ERR_FAIL_MSG("Downstream torque not implemented for this node");
    }
    virtual void apply_reaction(const DownstreamData &p_data) {
        ERR_FAIL_MSG("Reaction torque not implemented for this node");
        DEV_ASSERT(false);
    }
    virtual UpstreamData get_upstream_data() {
        ERR_FAIL_V_MSG({}, "Upstream is not implemented for this node");
        DEV_ASSERT(false);
    };
    virtual int get_output_count() const = 0;
    virtual void pre_update(float p_delta, const VehicleInputState &p_input_state) {}
    virtual void update(float p_delta, const VehicleInputState &p_input_state) {}
    static void _bind_methods() {}
    virtual String get_debug_text() const { return ""; }
    virtual String get_debugger_display_name() const {
        return name;
    }
    StringName get_name() const {
        return name;
    }

    friend class LNVehicle;
};