#pragma once

#include "constants.h"
#include <string>

class Config {
public:
    Config();
    ~Config();

    bool load_config();
    bool save_config() const;

    void set_enabled(bool enabled);
    bool is_enabled() const;
    
    void set_throttle_detection_enabled(bool enabled);
    bool is_throttle_detection_enabled() const;
    
    void set_elevator_control_enabled(bool enabled);
    bool is_elevator_control_enabled() const;
    
    void set_wake_category(glidestop::constants::WakeCategory category);
    glidestop::constants::WakeCategory get_wake_category() const;

    void reset_to_defaults();

private:
    bool m_enabled;
    bool m_throttle_detection_enabled;
    bool m_elevator_control_enabled;
    glidestop::constants::WakeCategory m_wake_category;
    
    std::string get_aircraft_directory() const;
    std::string get_config_file_path() const;
    bool create_default_config() const;
    bool parse_config_line(const std::string& line);
    std::string generate_config_content() const;
};