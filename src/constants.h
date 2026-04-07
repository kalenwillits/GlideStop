#pragma once

namespace glidestop {
namespace constants {

constexpr int XPLANE_STRING_BUFFER_SIZE = 256;
constexpr int XPLANE_PATH_BUFFER_SIZE = 512;

// Rotation speed defaults and limits (knots)
constexpr int DEFAULT_ROTATION_SPEED = 130;
constexpr int MIN_ROTATION_SPEED = 10;
constexpr int MAX_ROTATION_SPEED = 300;

// Brake system parameters
constexpr float THROTTLE_IDLE_THRESHOLD = 0.05f;  // ±5% around idle
constexpr float RUDDER_DEAD_ZONE = 0.05f;         // ±5% rudder dead zone
constexpr float BRAKE_EFFECTIVENESS_MAX = 1.0f;
constexpr float BRAKE_EFFECTIVENESS_MIN = 0.0f;

// Menu item IDs
enum MenuItems {
    MENU_TOGGLE_ENABLED = 2000,
    MENU_RELOAD_CONFIG = 2001,
    MENU_TOGGLE_THROTTLE_DETECTION = 2002,
    MENU_TOGGLE_ELEVATOR_CONTROL = 2003,
    MENU_ROTATION_SPEED_UP_10 = 2100,
    MENU_ROTATION_SPEED_UP_1 = 2101,
    MENU_ROTATION_SPEED_DISPLAY = 2102,
    MENU_ROTATION_SPEED_DOWN_1 = 2103,
    MENU_ROTATION_SPEED_DOWN_10 = 2104
};

}
}
