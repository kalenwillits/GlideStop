#include "brake_controller.h"
#include "XPLMUtilities.h"
#include <string>
#include <algorithm>
#include <cmath>

BrakeController::BrakeController() 
    : m_initialized(false)
    , m_enabled(false)
    , m_wake_category(glidestop::constants::DEFAULT_WAKE_CATEGORY)
{
    // Initialize datarefs to nullptr
    m_datarefs.override_toe_brakes = nullptr;
    m_datarefs.left_brake_ratio = nullptr;
    m_datarefs.right_brake_ratio = nullptr;
    m_datarefs.yoke_pitch_ratio = nullptr;
    m_datarefs.yoke_heading_ratio = nullptr;
    m_datarefs.throttle_ratio_all = nullptr;
    m_datarefs.true_airspeed_kts = nullptr;
}

BrakeController::~BrakeController() {
    cleanup();
}

bool BrakeController::initialize() {
    if (m_initialized) {
        return true;
    }

    // Find all required datarefs
    m_datarefs.override_toe_brakes = XPLMFindDataRef("sim/operation/override/override_toe_brakes");
    m_datarefs.left_brake_ratio = XPLMFindDataRef("sim/cockpit2/controls/left_brake_ratio");
    m_datarefs.right_brake_ratio = XPLMFindDataRef("sim/cockpit2/controls/right_brake_ratio");
    m_datarefs.yoke_pitch_ratio = XPLMFindDataRef("sim/joystick/yoke_pitch_ratio");
    m_datarefs.yoke_heading_ratio = XPLMFindDataRef("sim/joystick/yoke_heading_ratio");
    m_datarefs.throttle_ratio_all = XPLMFindDataRef("sim/cockpit2/engine/actuators/throttle_ratio_all");
    m_datarefs.true_airspeed_kts = XPLMFindDataRef("sim/cockpit2/gauges/indicators/true_airspeed_kts_pilot");

    // Check if all datarefs were found
    bool success = true;
    if (!m_datarefs.override_toe_brakes) {
        XPLMDebugString("GlideStop: ERROR - Could not find dataref: sim/operation/override/override_toe_brakes\n");
        success = false;
    }
    if (!m_datarefs.left_brake_ratio) {
        XPLMDebugString("GlideStop: ERROR - Could not find dataref: sim/cockpit2/controls/left_brake_ratio\n");
        success = false;
    }
    if (!m_datarefs.right_brake_ratio) {
        XPLMDebugString("GlideStop: ERROR - Could not find dataref: sim/cockpit2/controls/right_brake_ratio\n");
        success = false;
    }
    if (!m_datarefs.yoke_pitch_ratio) {
        XPLMDebugString("GlideStop: ERROR - Could not find dataref: sim/joystick/yoke_pitch_ratio\n");
        success = false;
    }
    if (!m_datarefs.yoke_heading_ratio) {
        XPLMDebugString("GlideStop: ERROR - Could not find dataref: sim/joystick/yoke_heading_ratio\n");
        success = false;
    }
    if (!m_datarefs.throttle_ratio_all) {
        XPLMDebugString("GlideStop: ERROR - Could not find dataref: sim/cockpit2/engine/actuators/throttle_ratio_all\n");
        success = false;
    }
    if (!m_datarefs.true_airspeed_kts) {
        XPLMDebugString("GlideStop: ERROR - Could not find dataref: sim/cockpit2/gauges/indicators/true_airspeed_kts_pilot\n");
        success = false;
    }

    if (success) {
        m_initialized = true;
        XPLMDebugString("GlideStop: Brake controller initialized successfully\n");
    } else {
        XPLMDebugString("GlideStop: ERROR - Failed to initialize brake controller\n");
    }

    return success;
}

void BrakeController::cleanup() {
    if (m_initialized && m_enabled) {
        // Disable brake override when cleaning up
        set_enabled(false);
    }
    m_initialized = false;
}

void BrakeController::update() {
    if (!m_initialized || !m_enabled) {
        return;
    }

    // Get current inputs
    float pitch_input = get_input_value(m_datarefs.yoke_pitch_ratio);
    float yaw_input = get_input_value(m_datarefs.yoke_heading_ratio);
    float airspeed_factor = get_airspeed_factor();
    
    // Calculate brake values based on inputs
    // Left brake: pitch input + negative yaw (left rudder)
    float left_brake = calculate_brake_value(pitch_input, -yaw_input, airspeed_factor);
    
    // Right brake: pitch input + positive yaw (right rudder) 
    float right_brake = calculate_brake_value(pitch_input, yaw_input, airspeed_factor);
    
    // If throttle is at idle, don't release brakes completely
    if (is_throttle_at_idle()) {
        // Maintain minimum brake pressure when throttle at idle
        left_brake = std::max<float>(left_brake, 0.1f * airspeed_factor);
        right_brake = std::max<float>(right_brake, 0.1f * airspeed_factor);
    }
    
    // Apply brake values
    XPLMSetDataf(m_datarefs.left_brake_ratio, left_brake);
    XPLMSetDataf(m_datarefs.right_brake_ratio, right_brake);
}

void BrakeController::set_enabled(bool enabled) {
    if (!m_initialized) {
        return;
    }
    
    if (m_enabled != enabled) {
        m_enabled = enabled;
        set_brake_override(enabled);
        
        if (!enabled) {
            // Release brakes when disabling
            XPLMSetDataf(m_datarefs.left_brake_ratio, 0.0f);
            XPLMSetDataf(m_datarefs.right_brake_ratio, 0.0f);
        }
        
        std::string log_msg = "GlideStop: " + std::string(enabled ? "Enabled" : "Disabled") + "\n";
        XPLMDebugString(log_msg.c_str());
    }
}

bool BrakeController::is_enabled() const {
    return m_enabled;
}

void BrakeController::set_wake_category(glidestop::constants::WakeCategory category) {
    if (static_cast<int>(category) >= 0 && 
        static_cast<int>(category) < static_cast<int>(glidestop::constants::WakeCategory::COUNT)) {
        m_wake_category = category;
        
        std::string log_msg = "GlideStop: Set wake category to ";
        log_msg += glidestop::constants::WAKE_CATEGORY_SHORT_NAMES[static_cast<int>(category)];
        log_msg += " (rotation speed: ";
        log_msg += std::to_string(get_rotation_speed());
        log_msg += " kt)\n";
        XPLMDebugString(log_msg.c_str());
    }
}

glidestop::constants::WakeCategory BrakeController::get_wake_category() const {
    return m_wake_category;
}

float BrakeController::get_rotation_speed() const {
    int category_index = static_cast<int>(m_wake_category);
    if (category_index >= 0 && category_index < glidestop::constants::NUM_WAKE_CATEGORIES) {
        return glidestop::constants::ROTATION_SPEEDS[category_index];
    }
    return glidestop::constants::ROTATION_SPEEDS[static_cast<int>(glidestop::constants::DEFAULT_WAKE_CATEGORY)];
}

float BrakeController::get_current_airspeed() const {
    if (!m_initialized || !m_datarefs.true_airspeed_kts) {
        return 0.0f;
    }
    return XPLMGetDataf(m_datarefs.true_airspeed_kts);
}

float BrakeController::get_airspeed_factor() const {
    float current_airspeed = get_current_airspeed();
    float rotation_speed = get_rotation_speed();
    
    // Validate inputs
    current_airspeed = std::max<float>(0.0f, current_airspeed);
    
    // Linear brake effectiveness: 100% at 0kt, 0% at rotation speed
    if (rotation_speed <= 0.0f) {
        XPLMDebugString("GlideStop: Warning - Invalid rotation speed, using full brake effectiveness\n");
        return 1.0f;
    }
    
    float factor = std::max<float>(0.0f, (rotation_speed - current_airspeed) / rotation_speed);
    return std::clamp<float>(factor, 
                            glidestop::constants::BRAKE_EFFECTIVENESS_MIN, 
                            glidestop::constants::BRAKE_EFFECTIVENESS_MAX);
}

float BrakeController::calculate_brake_value(float pitch_input, float yaw_input, float airspeed_factor) const {
    // Validate inputs
    pitch_input = std::clamp<float>(pitch_input, -1.0f, 1.0f);
    yaw_input = std::clamp<float>(yaw_input, -1.0f, 1.0f);
    airspeed_factor = std::clamp<float>(airspeed_factor, 0.0f, 1.0f);
    
    // Combine pitch and yaw inputs, apply airspeed factor
    float combined_input = pitch_input + yaw_input;
    float brake_value = airspeed_factor * combined_input;
    
    return std::clamp<float>(brake_value, 
                            glidestop::constants::BRAKE_EFFECTIVENESS_MIN,
                            glidestop::constants::BRAKE_EFFECTIVENESS_MAX);
}

float BrakeController::get_input_value(XPLMDataRef dataref) const {
    if (!dataref) {
        return 0.0f;
    }
    return XPLMGetDataf(dataref);
}

bool BrakeController::is_throttle_at_idle() const {
    if (!m_datarefs.throttle_ratio_all) {
        return false;
    }
    
    float throttle_position = XPLMGetDataf(m_datarefs.throttle_ratio_all);
    return std::abs(throttle_position) <= glidestop::constants::THROTTLE_IDLE_THRESHOLD;
}

void BrakeController::set_brake_override(bool override) {
    if (m_datarefs.override_toe_brakes) {
        XPLMSetDatai(m_datarefs.override_toe_brakes, override ? 1 : 0);
    }
}