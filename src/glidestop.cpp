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

// Aircraft tracking for config reloading

// Menu tracking
static int g_enabled_menu_item = -1;
static std::vector<int> g_wake_category_menu_items;

// Forward declarations
static void menu_handler(void* menu_ref, void* item_ref);
static float flight_loop_callback(float elapsed_since_last_call, float elapsed_since_last_flight_loop, 
                                 int counter, void* refcon);
static void create_menu_system();
static void load_aircraft_config();
static void update_menu_checkmarks();

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
    
    // Wake category section
    XPLMAppendMenuItem(g_submenu_id, "Wake Category:", nullptr, 0);
    g_wake_category_menu_items.reserve(NUM_WAKE_CATEGORIES);
    for (int i = 0; i < NUM_WAKE_CATEGORIES; ++i) {
        char label_buffer[128];
        std::snprintf(label_buffer, sizeof(label_buffer), "  %s - %.0f kt", 
                     WAKE_CATEGORY_NAMES[i], ROTATION_SPEEDS[i]);
        
        int item_id = XPLMAppendMenuItem(g_submenu_id, label_buffer, 
                                        reinterpret_cast<void*>(MENU_WAKE_CATEGORY_START + i), 1);
        g_wake_category_menu_items.push_back(item_id);
    }
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
    
    // Handle wake category selection
    if (item >= MENU_WAKE_CATEGORY_START && 
        item < MENU_WAKE_CATEGORY_START + NUM_WAKE_CATEGORIES) {
        int category_index = item - MENU_WAKE_CATEGORY_START;
        
        // Validate category index is within bounds
        if (category_index >= 0 && category_index < NUM_WAKE_CATEGORIES) {
            WakeCategory category = static_cast<WakeCategory>(category_index);
            
            if (g_config && g_brake_controller) {
                g_config->set_wake_category(category);
                g_brake_controller->set_wake_category(category);
                update_menu_checkmarks();
                
                // Save configuration after category change
                g_config->save_config();
            }
        }
    }
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
            g_brake_controller->set_wake_category(g_config->get_wake_category());
        }
        
        update_menu_checkmarks();
    }
}

static void update_menu_checkmarks()
{
    using namespace glidestop::constants;
    
    if (!g_config || !g_submenu_id) return;
    
    // Update enabled/disabled menu item text
    if (g_enabled_menu_item >= 0) {
        const char* menu_text = g_config->is_enabled() ? "✓ Disable GlideStop" : "Enable GlideStop";
        XPLMSetMenuItemName(g_submenu_id, g_enabled_menu_item, menu_text, 0);
    }
    
    // Update wake category checkmarks
    WakeCategory current_category = g_config->get_wake_category();
    int current_category_index = static_cast<int>(current_category);
    
    // Validate current category is within bounds
    if (current_category_index < 0 || current_category_index >= NUM_WAKE_CATEGORIES) {
        current_category_index = static_cast<int>(DEFAULT_WAKE_CATEGORY);
    }
    
    for (int i = 0; i < NUM_WAKE_CATEGORIES && i < static_cast<int>(g_wake_category_menu_items.size()); ++i) {
        XPLMCheckMenuItem(g_submenu_id, g_wake_category_menu_items[i], 
                        (i == current_category_index) ? xplm_Menu_Checked : xplm_Menu_Unchecked);
    }
}