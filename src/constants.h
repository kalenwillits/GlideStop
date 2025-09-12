#pragma once

namespace glidestop {
namespace constants {

constexpr int XPLANE_STRING_BUFFER_SIZE = 256;
constexpr int XPLANE_PATH_BUFFER_SIZE = 512;

// Wake turbulence categories and rotation speeds
enum class WakeCategory {
    LIGHT = 0,    // L - Light aircraft (≤7,000 kg)
    MEDIUM = 1,   // M - Medium aircraft (7,000-136,000 kg)  
    HEAVY = 2,    // H - Heavy aircraft (136,000+ kg)
    SUPER = 3,    // J - Super aircraft (A380-class)
    COUNT = 4
};

// Rotation speeds in knots for each wake category
constexpr float ROTATION_SPEEDS[] = {
    65.0f,   // Light - Small GA/turboprops
    130.0f,  // Medium - Regional jets/smaller airliners
    155.0f,  // Heavy - Large transport jets
    165.0f   // Super - A380-class
};

constexpr int NUM_WAKE_CATEGORIES = sizeof(ROTATION_SPEEDS) / sizeof(ROTATION_SPEEDS[0]);
constexpr WakeCategory DEFAULT_WAKE_CATEGORY = WakeCategory::MEDIUM;

// Brake system parameters
constexpr float THROTTLE_IDLE_THRESHOLD = 0.05f;  // ±5% around idle
constexpr float BRAKE_EFFECTIVENESS_MAX = 1.0f;
constexpr float BRAKE_EFFECTIVENESS_MIN = 0.0f;

// Menu item IDs
enum MenuItems {
    MENU_TOGGLE_ENABLED = 2000,
    MENU_RELOAD_CONFIG = 2001,
    MENU_TOGGLE_THROTTLE_DETECTION = 2002,
    MENU_TOGGLE_ELEVATOR_CONTROL = 2003,
    MENU_WAKE_CATEGORY_START = 2100
};

// Wake category names for UI
constexpr const char* WAKE_CATEGORY_NAMES[] = {
    "Light (≤7,000 kg)",
    "Medium (7,000-136,000 kg)", 
    "Heavy (136,000+ kg)",
    "Super (A380-class)"
};

constexpr const char* WAKE_CATEGORY_SHORT_NAMES[] = {
    "L", "M", "H", "J"
};

}
}