#include "config.h"
#include "XPLMUtilities.h"
#include "XPLMPlanes.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <filesystem>

Config::Config() {
    reset_to_defaults();
}

Config::~Config() {
}

void Config::reset_to_defaults() {
    m_enabled = false;
    m_throttle_detection_enabled = true;
    m_elevator_control_enabled = true;
    m_rotation_speed = glidestop::constants::DEFAULT_ROTATION_SPEED;
}

bool Config::load_config() {
    std::string config_path = get_config_file_path();

    std::ifstream config_file(config_path);
    if (!config_file.is_open()) {
        std::string log_msg = "GlideStop: Config file not found: " + config_path + ", creating default config\n";
        XPLMDebugString(log_msg.c_str());

        reset_to_defaults();

        if (!create_default_config()) {
            return true; // Use defaults if we can't create config
        }

        // Try to open the newly created file
        config_file.open(config_path);
        if (!config_file.is_open()) {
            std::string error_msg = "GlideStop: WARNING - Could not open newly created config file, using defaults\n";
            XPLMDebugString(error_msg.c_str());
            return true; // Use defaults
        }
    }

    reset_to_defaults();

    std::string line;
    while (std::getline(config_file, line)) {
        if (!line.empty() && line[0] != '#') {
            parse_config_line(line);
        }
    }

    config_file.close();

    std::string log_msg = "GlideStop: Loaded config from: " + config_path + "\n";
    XPLMDebugString(log_msg.c_str());
    return true;
}

bool Config::save_config() const {
    std::string config_path = get_config_file_path();

    // Create directory if it doesn't exist
    std::filesystem::path config_file_path(config_path);
    std::filesystem::path config_dir = config_file_path.parent_path();

    try {
        if (!std::filesystem::exists(config_dir)) {
            std::filesystem::create_directories(config_dir);
        }
    } catch (const std::exception& e) {
        std::string error_msg = "GlideStop: ERROR - Could not create config directory: ";
        error_msg += e.what();
        error_msg += "\n";
        XPLMDebugString(error_msg.c_str());
        return false;
    }

    std::ofstream config_file(config_path);
    if (!config_file.is_open()) {
        std::string error_msg = "GlideStop: ERROR - Could not write config file: " + config_path + "\n";
        XPLMDebugString(error_msg.c_str());
        return false;
    }

    config_file << generate_config_content();
    config_file.close();

    std::string log_msg = "GlideStop: Saved config to: " + config_path + "\n";
    XPLMDebugString(log_msg.c_str());
    return true;
}

void Config::set_enabled(bool enabled) {
    m_enabled = enabled;
}

bool Config::is_enabled() const {
    return m_enabled;
}

void Config::set_throttle_detection_enabled(bool enabled) {
    m_throttle_detection_enabled = enabled;
}

bool Config::is_throttle_detection_enabled() const {
    return m_throttle_detection_enabled;
}

void Config::set_elevator_control_enabled(bool enabled) {
    m_elevator_control_enabled = enabled;
}

bool Config::is_elevator_control_enabled() const {
    return m_elevator_control_enabled;
}

void Config::set_rotation_speed(int speed) {
    m_rotation_speed = std::clamp(speed,
        glidestop::constants::MIN_ROTATION_SPEED,
        glidestop::constants::MAX_ROTATION_SPEED);
}

int Config::get_rotation_speed() const {
    return m_rotation_speed;
}

std::string Config::get_aircraft_directory() const {
    char filename[glidestop::constants::XPLANE_PATH_BUFFER_SIZE];
    char path[glidestop::constants::XPLANE_PATH_BUFFER_SIZE];

    XPLMGetNthAircraftModel(0, filename, path);

    std::string aircraft_path(path);
    if (!aircraft_path.empty()) {
        // The path includes the .acf filename, extract just the directory
        size_t last_slash = aircraft_path.find_last_of("/\\");
        if (last_slash != std::string::npos) {
            aircraft_path = aircraft_path.substr(0, last_slash);
        }
    }

    return aircraft_path;
}

std::string Config::get_config_file_path() const {
    std::string aircraft_dir = get_aircraft_directory();
    return aircraft_dir + "/GlideStop.cfg";
}

bool Config::create_default_config() const {
    std::string config_file = get_config_file_path();

    // Create directory if it doesn't exist
    std::filesystem::path config_path(config_file);
    std::filesystem::path config_dir = config_path.parent_path();

    if (!config_dir.empty()) {
        try {
            std::filesystem::create_directories(config_dir);
        } catch (const std::exception& e) {
            std::string error_msg = "GlideStop: ERROR - Failed to create directory " + config_dir.string() + ": " + e.what() + "\n";
            XPLMDebugString(error_msg.c_str());
            return false;
        }
    }

    // Create default config file with default settings
    std::ofstream file(config_file);
    if (!file.is_open()) {
        std::string error_msg = "GlideStop: ERROR - Could not create config file: " + config_file + "\n";
        XPLMDebugString(error_msg.c_str());
        return false;
    }

    // Write default config with comments
    file << "# GlideStop Configuration File\n";
    file << "# \n";
    file << "# This file stores settings for the GlideStop brake assistance system.\n";
    file << "# \n";
    file << "# enabled: true/false - Whether GlideStop is active\n";
    file << "# rotation_speed: integer (knots) - Speed at which brakes reach zero effectiveness\n";
    file << "# \n";
    file << "enabled=false\n";
    file << "rotation_speed=" << glidestop::constants::DEFAULT_ROTATION_SPEED << "\n";

    file.close();

    std::string log_msg = "GlideStop: Created default config file: " + config_file + "\n";
    XPLMDebugString(log_msg.c_str());

    return true;
}

bool Config::parse_config_line(const std::string& line) {
    // Remove leading/trailing whitespace
    std::string trimmed_line = line;
    size_t start = trimmed_line.find_first_not_of(" \t\r\n");
    if (start != std::string::npos) {
        trimmed_line = trimmed_line.substr(start);
    }
    size_t end = trimmed_line.find_last_not_of(" \t\r\n");
    if (end != std::string::npos) {
        trimmed_line = trimmed_line.substr(0, end + 1);
    }

    if (trimmed_line.empty()) {
        return true;
    }

    // Parse key=value format
    size_t equals_pos = trimmed_line.find('=');
    if (equals_pos == std::string::npos) {
        return false;
    }

    std::string key = trimmed_line.substr(0, equals_pos);
    std::string value = trimmed_line.substr(equals_pos + 1);

    // Trim key and value
    key.erase(key.find_last_not_of(" \t") + 1);
    key.erase(0, key.find_first_not_of(" \t"));
    value.erase(value.find_last_not_of(" \t") + 1);
    value.erase(0, value.find_first_not_of(" \t"));

    if (key == "enabled") {
        m_enabled = (value == "true" || value == "1");
    } else if (key == "throttle_detection") {
        m_throttle_detection_enabled = (value == "true" || value == "1");
    } else if (key == "elevator_control") {
        m_elevator_control_enabled = (value == "true" || value == "1");
    } else if (key == "rotation_speed") {
        try {
            int speed = std::stoi(value);
            m_rotation_speed = std::clamp(speed,
                glidestop::constants::MIN_ROTATION_SPEED,
                glidestop::constants::MAX_ROTATION_SPEED);
        } catch (const std::exception&) {
            m_rotation_speed = glidestop::constants::DEFAULT_ROTATION_SPEED;
            XPLMDebugString("GlideStop: Invalid rotation speed value, using default\n");
        }
    } else if (key == "wake_category") {
        // Backward compatibility: convert old wake_category index to rotation speed
        constexpr int legacy_speeds[] = { 65, 130, 155, 165, 30 };
        try {
            int category = std::stoi(value);
            if (category >= 0 && category < 5) {
                m_rotation_speed = legacy_speeds[category];
            } else {
                m_rotation_speed = glidestop::constants::DEFAULT_ROTATION_SPEED;
            }
            XPLMDebugString("GlideStop: Migrated legacy wake_category to rotation_speed\n");
        } catch (const std::exception&) {
            m_rotation_speed = glidestop::constants::DEFAULT_ROTATION_SPEED;
            XPLMDebugString("GlideStop: Invalid legacy wake_category, using default\n");
        }
    } else {
        return false;
    }

    return true;
}

std::string Config::generate_config_content() const {
    std::ostringstream content;

    content << "# GlideStop Configuration File\n";
    content << "# Automatic brake control for aircraft without toe braking hardware\n";
    content << "#\n";
    content << "# rotation_speed: Rotation speed in knots ("
            << glidestop::constants::MIN_ROTATION_SPEED << "-"
            << glidestop::constants::MAX_ROTATION_SPEED << ")\n";
    content << "# Brakes are at full effectiveness at 0 kt and zero at rotation speed\n";
    content << "\n";

    content << "enabled=" << (m_enabled ? "true" : "false") << "\n";
    content << "throttle_detection=" << (m_throttle_detection_enabled ? "true" : "false") << "\n";
    content << "elevator_control=" << (m_elevator_control_enabled ? "true" : "false") << "\n";
    content << "rotation_speed=" << m_rotation_speed << "\n";

    return content.str();
}
