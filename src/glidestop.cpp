#include "XPLMPlugin.h"
#include "XPLMUtilities.h"
#include "XPLMMenus.h"
#include "XPLMDataAccess.h"
#include "XPLMPlanes.h"
#include "XPLMProcessing.h"

#include "config.h"
#include "brake_controller.h"
#include "constants.h"

#include <string>
#include <vector>
#include <cstring>
#include <memory>
#include <cstdio>

// Global plugin state
static XPLMMenuID g_menu_id = nullptr;
static XPLMMenuID g_submenu_id = nullptr;
static std::unique_ptr<Config> g_config;
static std::unique_ptr<BrakeController> g_brake_controller;

// Menu tracking
static int g_enabled_menu_item = -1;
static int g_throttle_detection_menu_item = -1;
static int g_elevator_control_menu_item = -1;
static int g_rotation_speed_display_item = -1;

// Forward declarations
static void menu_handler(void* menu_ref, void* item_ref);
static float flight_loop_callback(float elapsed_since_last_call, float elapsed_since_last_flight_loop,
                                 int counter, void* refcon);
static void create_menu_system();
static void load_aircraft_config();
static void update_menu_checkmarks();
static void adjust_rotation_speed(int delta);

PLUGIN_API int XPluginStart(char* out_name, char* out_sig, char* out_desc)
{
    using namespace glidestop::constants;

    std::string name = "GlideStop";
    std::string sig = "glidestop.plugin";
    std::string desc = "Aircraft brake control plugin for improved controllability";

    // Safely copy strings with proper null termination
    std::strncpy(out_name, name.c_str(), XPLANE_STRING_BUFFER_SIZE - 1);
    std::strncpy(out_sig, sig.c_str(), XPLANE_STRING_BUFFER_SIZE - 1);
    std::strncpy(out_desc, desc.c_str(), XPLANE_STRING_BUFFER_SIZE - 1);
    out_name[XPLANE_STRING_BUFFER_SIZE - 1] = '\0';
    out_sig[XPLANE_STRING_BUFFER_SIZE - 1] = '\0';
    out_desc[XPLANE_STRING_BUFFER_SIZE - 1] = '\0';

    // Initialize components
    g_config = std::make_unique<Config>();
    g_brake_controller = std::make_unique<BrakeController>();

    if (!g_brake_controller->initialize()) {
        XPLMDebugString("GlideStop: ERROR - Failed to initialize brake controller\n");
        return 0;
    }

    // Create plugin menu
    g_menu_id = XPLMFindPluginsMenu();
    if (g_menu_id) {
        int menu_item = XPLMAppendMenuItem(g_menu_id, "GlideStop", nullptr, 1);
        g_submenu_id = XPLMCreateMenu("GlideStop", g_menu_id, menu_item, menu_handler, nullptr);
        create_menu_system();
    }

    // Register flight loop callback for continuous brake processing
    XPLMRegisterFlightLoopCallback(flight_loop_callback, -1.0f, nullptr);

    XPLMDebugString("GlideStop: Plugin started successfully\n");
    return 1;
}

PLUGIN_API void XPluginStop(void)
{
    // Unregister flight loop callback
    XPLMUnregisterFlightLoopCallback(flight_loop_callback, nullptr);

    // Cleanup components
    if (g_brake_controller) {
        g_brake_controller->cleanup();
        g_brake_controller.reset();
    }

    g_config.reset();

    XPLMDebugString("GlideStop: Plugin stopped\n");
}

PLUGIN_API int XPluginEnable(void)
{
    load_aircraft_config();
    return 1;
}

PLUGIN_API void XPluginDisable(void)
{
    // Save current configuration when disabled
    if (g_config) {
        g_config->save_config();
    }

    // Ensure brake controller is disabled
    if (g_brake_controller) {
        g_brake_controller->set_enabled(false);
    }
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID, int msg, void*)
{
    if (msg == XPLM_MSG_PLANE_LOADED) {
        load_aircraft_config();
    }
}

static void create_menu_system()
{
    using namespace glidestop::constants;

    if (!g_submenu_id) return;

    // Add enable/disable toggle
    g_enabled_menu_item = XPLMAppendMenuItem(g_submenu_id, "Enable GlideStop",
                                           reinterpret_cast<void*>(MENU_TOGGLE_ENABLED), 1);

    // Add reload config option
    XPLMAppendMenuItem(g_submenu_id, "Reload Configuration",
                      reinterpret_cast<void*>(MENU_RELOAD_CONFIG), 1);

    // Add separator
    XPLMAppendMenuSeparator(g_submenu_id);

    // Control options section
    XPLMAppendMenuItem(g_submenu_id, "Control Options:", nullptr, 0);
    g_throttle_detection_menu_item = XPLMAppendMenuItem(g_submenu_id, "  Throttle Idle Detection",
                                                       reinterpret_cast<void*>(MENU_TOGGLE_THROTTLE_DETECTION), 1);
    g_elevator_control_menu_item = XPLMAppendMenuItem(g_submenu_id, "  Elevator Brake Control",
                                                     reinterpret_cast<void*>(MENU_TOGGLE_ELEVATOR_CONTROL), 1);

    // Add separator
    XPLMAppendMenuSeparator(g_submenu_id);

    // Rotation speed section
    XPLMAppendMenuItem(g_submenu_id, "Rotation Speed (kt):", nullptr, 0);
    XPLMAppendMenuItem(g_submenu_id, "  +10",
                      reinterpret_cast<void*>(MENU_ROTATION_SPEED_UP_10), 1);
    XPLMAppendMenuItem(g_submenu_id, "  +1",
                      reinterpret_cast<void*>(MENU_ROTATION_SPEED_UP_1), 1);
    g_rotation_speed_display_item = XPLMAppendMenuItem(g_submenu_id, "  --- kt",
                      reinterpret_cast<void*>(MENU_ROTATION_SPEED_DISPLAY), 1);
    XPLMAppendMenuItem(g_submenu_id, "  -1",
                      reinterpret_cast<void*>(MENU_ROTATION_SPEED_DOWN_1), 1);
    XPLMAppendMenuItem(g_submenu_id, "  -10",
                      reinterpret_cast<void*>(MENU_ROTATION_SPEED_DOWN_10), 1);
}

static void menu_handler(void*, void* item_ref)
{
    using namespace glidestop::constants;

    intptr_t item = reinterpret_cast<intptr_t>(item_ref);

    if (item == MENU_TOGGLE_ENABLED) {
        if (g_config && g_brake_controller) {
            bool new_enabled = !g_config->is_enabled();
            g_config->set_enabled(new_enabled);
            g_brake_controller->set_enabled(new_enabled);
            update_menu_checkmarks();

            // Save configuration after toggle
            g_config->save_config();
        }
        return;
    }

    if (item == MENU_RELOAD_CONFIG) {
        XPLMDebugString("GlideStop: Reloading configuration\n");
        load_aircraft_config();
        return;
    }

    if (item == MENU_TOGGLE_THROTTLE_DETECTION) {
        if (g_brake_controller && g_config) {
            bool new_enabled = !g_brake_controller->is_throttle_detection_enabled();
            g_brake_controller->set_throttle_detection_enabled(new_enabled);
            g_config->set_throttle_detection_enabled(new_enabled);
            g_config->save_config();
            update_menu_checkmarks();
        }
        return;
    }

    if (item == MENU_TOGGLE_ELEVATOR_CONTROL) {
        if (g_brake_controller && g_config) {
            bool new_enabled = !g_brake_controller->is_elevator_control_enabled();
            g_brake_controller->set_elevator_control_enabled(new_enabled);
            g_config->set_elevator_control_enabled(new_enabled);
            g_config->save_config();
            update_menu_checkmarks();
        }
        return;
    }

    if (item == MENU_ROTATION_SPEED_UP_10) {
        adjust_rotation_speed(10);
        return;
    }
    if (item == MENU_ROTATION_SPEED_UP_1) {
        adjust_rotation_speed(1);
        return;
    }
    if (item == MENU_ROTATION_SPEED_DOWN_1) {
        adjust_rotation_speed(-1);
        return;
    }
    if (item == MENU_ROTATION_SPEED_DOWN_10) {
        adjust_rotation_speed(-10);
        return;
    }
}

static void adjust_rotation_speed(int delta)
{
    if (!g_config || !g_brake_controller) return;

    int new_speed = g_config->get_rotation_speed() + delta;
    g_config->set_rotation_speed(new_speed);
    g_brake_controller->set_rotation_speed(g_config->get_rotation_speed());
    g_config->save_config();
    update_menu_checkmarks();
}

static float flight_loop_callback(float, float, int, void*)
{
    // Update brake controller
    if (g_brake_controller) {
        g_brake_controller->update();
    }

    // Call again next frame
    return -1.0f;
}

static void load_aircraft_config()
{
    XPLMDebugString("GlideStop: Loading aircraft configuration\n");

    if (g_config) {
        g_config->load_config();

        // Apply config to brake controller
        if (g_brake_controller) {
            g_brake_controller->set_enabled(g_config->is_enabled());
            g_brake_controller->set_throttle_detection_enabled(g_config->is_throttle_detection_enabled());
            g_brake_controller->set_elevator_control_enabled(g_config->is_elevator_control_enabled());
            g_brake_controller->set_rotation_speed(g_config->get_rotation_speed());
        }

        update_menu_checkmarks();
    }
}

static void update_menu_checkmarks()
{
    if (!g_config || !g_submenu_id) return;

    // Update enabled/disabled menu item text
    if (g_enabled_menu_item >= 0) {
        const char* menu_text = g_config->is_enabled() ? "✓ Disable GlideStop" : "Enable GlideStop";
        XPLMSetMenuItemName(g_submenu_id, g_enabled_menu_item, menu_text, 0);
    }

    // Update control option checkmarks
    if (g_brake_controller) {
        if (g_throttle_detection_menu_item >= 0) {
            const char* throttle_text = g_brake_controller->is_throttle_detection_enabled() ?
                "✓ Throttle Idle Detection" : "  Throttle Idle Detection";
            XPLMSetMenuItemName(g_submenu_id, g_throttle_detection_menu_item, throttle_text, 0);
        }

        if (g_elevator_control_menu_item >= 0) {
            const char* elevator_text = g_brake_controller->is_elevator_control_enabled() ?
                "✓ Elevator Brake Control" : "  Elevator Brake Control";
            XPLMSetMenuItemName(g_submenu_id, g_elevator_control_menu_item, elevator_text, 0);
        }
    }

    // Update rotation speed display
    if (g_rotation_speed_display_item >= 0) {
        char speed_label[64];
        std::snprintf(speed_label, sizeof(speed_label), "  [ %d kt ]", g_config->get_rotation_speed());
        XPLMSetMenuItemName(g_submenu_id, g_rotation_speed_display_item, speed_label, 0);
    }
}
