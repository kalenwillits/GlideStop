#include "brake_controller.h"
#include "XPLMUtilities.h"
#include <string>
#include <algorithm>
#include <cmath>

BrakeController::BrakeController()
    : m_initialized(false)
    , m_enabled(false)
    , m_throttle_detection_enabled(true)
    , m_elevator_control_enabled(true)
    , m_wake_category(glidestop::constants::DEFAULT_WAKE_CATEGORY)
    , m_previous_left_brake(0.0f)
    , m_previous_right_brake(0.0f)
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

    // Get current inputs - respect toggle settings
    float pitch_input = m_elevator_control_enabled ? get_input_value(m_datarefs.yoke_pitch_ratio) : 0.0f;
    float yaw_input = get_input_value(m_datarefs.yoke_heading_ratio);  // Rudder always enabled

    // Calculate raw brake values from inputs (0.0-1.0 range, no airspeed factor)
    // Left brake: pitch input + negative yaw (left rudder)
    float left_brake = calculate_brake_value(pitch_input, -yaw_input);

    // Right brake: pitch input + positive yaw (right rudder)
    float right_brake = calculate_brake_value(pitch_input, yaw_input);

    // Apply throttle idle brake hold logic on RAW values (CORRECTED BEHAVIOR)
    if (m_throttle_detection_enabled && is_throttle_at_idle()) {
        // When throttle is AT idle: brakes can only increase, never decrease (hold position)
        left_brake = std::max<float>(left_brake, m_previous_left_brake);
        right_brake = std::max<float>(right_brake, m_previous_right_brake);
    }
    // When throttle is ABOVE idle: brakes work normally (no special logic)

    // Store RAW brake values for next iteration (before airspeed factor)
    m_previous_left_brake = left_brake;
    m_previous_right_brake = right_brake;

    // Apply airspeed factor LAST (as specified)
    float airspeed_factor = get_airspeed_factor();
    left_brake *= airspeed_factor;
    right_brake *= airspeed_factor;

    // CRITICAL: Clamp final brake values to prevent unnatural forces
    left_brake = std::clamp<float>(left_brake, 0.0f, 1.0f);
    right_brake = std::clamp<float>(right_brake, 0.0f, 1.0f);

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
            // Reset brake hold state
            m_previous_left_brake = 0.0f;
            m_previous_right_brake = 0.0f;
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

float BrakeController::calculate_brake_value(float pitch_input, float yaw_input) const {
    // Validate inputs
    pitch_input = std::clamp<float>(pitch_input, -1.0f, 1.0f);
    yaw_input = std::clamp<float>(yaw_input, -1.0f, 1.0f);

    // Combine pitch and yaw inputs (raw brake value without airspeed factor)
    float combined_input = pitch_input + yaw_input;

    return std::clamp<float>(combined_input, 0.0f, 1.0f);
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

void BrakeController::set_throttle_detection_enabled(bool enabled) {
    m_throttle_detection_enabled = enabled;
    std::string log_msg = "GlideStop: Throttle idle detection " + 
                          std::string(enabled ? "enabled" : "disabled") + "\n";
    XPLMDebugString(log_msg.c_str());
}

bool BrakeController::is_throttle_detection_enabled() const {
    return m_throttle_detection_enabled;
}

void BrakeController::set_elevator_control_enabled(bool enabled) {
    m_elevator_control_enabled = enabled;
    std::string log_msg = "GlideStop: Elevator brake control " + 
                          std::string(enabled ? "enabled" : "disabled") + "\n";
    XPLMDebugString(log_msg.c_str());
}

bool BrakeController::is_elevator_control_enabled() const {
    return m_elevator_control_enabled;
}