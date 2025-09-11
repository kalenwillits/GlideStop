#include "config.h"
#include "XPLMUtilities.h"
#include "XPLMPlanes.h"
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
    m_wake_category = glidestop::constants::DEFAULT_WAKE_CATEGORY;
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

void Config::set_wake_category(glidestop::constants::WakeCategory category) {
    if (static_cast<int>(category) >= 0 && 
        static_cast<int>(category) < static_cast<int>(glidestop::constants::WakeCategory::COUNT)) {
        m_wake_category = category;
    }
}

glidestop::constants::WakeCategory Config::get_wake_category() const {
    return m_wake_category;
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
    if (aircraft_dir.empty()) {
        return "GlideStop.cfg"; // Fallback to current directory
    }
    
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
    file << "# wake_category: LIGHT/MEDIUM/HEAVY - Aircraft wake turbulence category\n";
    file << "# \n";
    file << "enabled=false\n";
    
    // Convert wake category enum to string
    std::string wake_category_str;
    switch (glidestop::constants::DEFAULT_WAKE_CATEGORY) {
        case glidestop::constants::WakeCategory::LIGHT:
            wake_category_str = "LIGHT";
            break;
        case glidestop::constants::WakeCategory::MEDIUM:
            wake_category_str = "MEDIUM";
            break;
        case glidestop::constants::WakeCategory::HEAVY:
            wake_category_str = "HEAVY";
            break;
        case glidestop::constants::WakeCategory::SUPER:
            wake_category_str = "SUPER";
            break;
        case glidestop::constants::WakeCategory::COUNT:
            // Should never happen, but handle it gracefully
            wake_category_str = "MEDIUM";
            break;
    }
    
    file << "wake_category=" << wake_category_str << "\n";
    
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
    } else if (key == "wake_category") {
        try {
            int category_index = std::stoi(value);
            if (category_index >= 0 && category_index < glidestop::constants::NUM_WAKE_CATEGORIES) {
                m_wake_category = static_cast<glidestop::constants::WakeCategory>(category_index);
            } else {
                // Invalid category, use default
                m_wake_category = glidestop::constants::DEFAULT_WAKE_CATEGORY;
                std::string log_msg = "GlideStop: Invalid wake category " + std::to_string(category_index) + 
                                     ", using default\n";
                XPLMDebugString(log_msg.c_str());
            }
        } catch (const std::exception&) {
            // Invalid number, use default
            m_wake_category = glidestop::constants::DEFAULT_WAKE_CATEGORY;
            XPLMDebugString("GlideStop: Invalid wake category value, using default\n");
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
    content << "# Wake Categories:\n";
    content << "# 0 = Light (≤7,000 kg) - Rotation speed: 65 kt\n";
    content << "# 1 = Medium (7,000-136,000 kg) - Rotation speed: 130 kt\n";
    content << "# 2 = Heavy (136,000+ kg) - Rotation speed: 155 kt\n";
    content << "# 3 = Super (A380-class) - Rotation speed: 165 kt\n";
    content << "\n";
    
    content << "enabled=" << (m_enabled ? "true" : "false") << "\n";
    content << "wake_category=" << static_cast<int>(m_wake_category) << "\n";
    
    return content.str();
}