#include "shaft.h"

class LNVehicleGearbox : public LNVehicleShaft {
    GDCLASS(LNVehicleGearbox, LNVehicleShaft);
    int current_gear = 0;
public:
    virtual bool has_input() const override;
    virtual int get_output_count() const override;
    virtual UpstreamData get_upstream_data() override;
    virtual void apply_downstream(const DownstreamData &p_data) override;
    virtual void pre_update(float p_delta, const VehicleInputState &p_input_state) override;
    virtual String get_debugger_display_name() const override;
    virtual String get_debug_text() const override;
    static void _bind_methods() {}

    int get_current_gear() const;
};