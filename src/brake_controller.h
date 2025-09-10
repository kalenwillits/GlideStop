#pragma once

#include "XPLMDataAccess.h"
#include "constants.h"

class BrakeController {
public:
    BrakeController();
    ~BrakeController();

    bool initialize();
    void cleanup();
    
    void update();
    
    void set_enabled(bool enabled);
    bool is_enabled() const;
    
    void set_wake_category(glidestop::constants::WakeCategory category);
    glidestop::constants::WakeCategory get_wake_category() const;
    
    float get_rotation_speed() const;
    float get_current_airspeed() const;
    float get_airspeed_factor() const;

private:
    struct DataRefs {
        XPLMDataRef override_toe_brakes;    // sim/operation/override/override_toe_brakes
        XPLMDataRef left_brake_ratio;       // sim/cockpit2/controls/left_brake_ratio
        XPLMDataRef right_brake_ratio;      // sim/cockpit2/controls/right_brake_ratio
        XPLMDataRef yoke_pitch_ratio;       // sim/joystick/yoke_pitch_ratio
        XPLMDataRef yoke_roll_ratio;        // sim/joystick/yoke_roll_ratio (rudder input)
        XPLMDataRef throttle_ratio_all;     // sim/cockpit2/engine/actuators/throttle_ratio_all
        XPLMDataRef true_airspeed_kts;      // sim/cockpit2/gauges/indicators/true_airspeed_kts_pilot
    };

    DataRefs m_datarefs;
    bool m_initialized;
    bool m_enabled;
    glidestop::constants::WakeCategory m_wake_category;
    
    float calculate_brake_value(float pitch_input, float yaw_input, float airspeed_factor) const;
    float get_input_value(XPLMDataRef dataref) const;
    bool is_throttle_at_idle() const;
    void set_brake_override(bool override);
};