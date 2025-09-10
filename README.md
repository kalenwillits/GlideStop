# GlideStop Plugin for X-Plane 12

GlideStop is an X-Plane 12 plugin that improves aircraft brake controllability when no toe braking hardware exists. It automatically controls aircraft brakes based on rudder and stick inputs, with brake effectiveness that varies by airspeed and aircraft wake category.

## Features

- **Automatic Brake Control**: No hardware modifications needed
  - Left rudder deflection → Left brake activation
  - Right rudder deflection → Right brake activation  
  - Pull back on stick → Both brakes activation
  - Combined inputs use maximum value

- **Airspeed-Based Effectiveness**: Realistic brake behavior
  - 100% effectiveness at 0 knots
  - Linear reduction to 0% at aircraft rotation speed
  - Prevents unrealistic braking at high speeds

- **Aircraft Wake Categories**: Proper rotation speeds per aircraft type
  - Light (≤7,000 kg): 65 kt rotation speed
  - Medium (7,000-136,000 kg): 130 kt rotation speed
  - Heavy (136,000+ kg): 155 kt rotation speed
  - Super (A380-class): 165 kt rotation speed

- **Throttle-Idle Detection**: Enhanced realism
  - Brakes don't fully release when throttle at idle (±1%)
  - Simulates pilot maintaining brake pressure during taxi

- **Per-Aircraft Configuration**: Settings automatically saved per aircraft

## Code Structure

### Core Components

- **`src/glidestop.cpp`** - Main plugin entry point
  - Implements X-Plane plugin callbacks and flight loop
  - Manages menu system and user interaction
  - Coordinates between brake controller and configuration

- **`src/brake_controller.h/cpp`** - Core brake logic
  - Manages X-Plane dataref access for brake controls
  - Implements brake calculation based on control inputs
  - Handles airspeed-based effectiveness and throttle detection
  - Uses datarefs: `override_toe_brakes`, `left/right_brake_ratio`, `yoke_pitch_ratio`, etc.

- **`src/config.h/cpp`** - Configuration management
  - Loads and saves per-aircraft `GlideStop.cfg` files
  - Manages enabled/disabled state and wake category selection
  - Uses `XPLMGetNthAircraftModel(0, ...)` for aircraft detection

- **`src/constants.h`** - Wake categories and brake parameters
  - Defines wake category enumeration and rotation speeds
  - Contains brake effectiveness parameters and menu constants

### Architecture

The plugin follows a modular design:

1. **Plugin Layer** (`glidestop.cpp`) - X-Plane SDK interface and flight loop
2. **Controller Layer** (`brake_controller`) - Real-time brake calculation and control
3. **Configuration Layer** (`config`) - Persistent settings management
4. **Constants Layer** (`constants`) - Shared definitions and parameters

## Building

### Prerequisites

- CMake 3.16 or higher
- C++17 compatible compiler
- X-Plane 12 SDK

### Quick Start (Recommended)

For first-time setup and building:

```bash
# Linux/Mac
./build-all.sh

# Windows
build-all.bat
```

These scripts will automatically download the X-Plane SDK, build the plugin, and provide installation instructions.

### Manual Setup & Build

#### Setup SDK

Option 1: Use setup scripts:
```bash
# Linux/Mac
./setup-sdk.sh

# Windows  
setup-sdk.bat
```

Option 2: Manual setup:
1. Download the X-Plane SDK from [developer.x-plane.com](https://developer.x-plane.com/sdk/plugin-sdk-downloads/)
2. Extract to `SDK/` folder in project directory
3. Alternatively, set `XPLANE_SDK_PATH` environment variable

#### Build Commands

```bash
# Quick build (debug)
./build.sh

# Release build with package
./build.sh release

# Windows
build.bat
build.bat release

# Manual CMake (advanced)
mkdir build && cd build
cmake ..
cmake --build .
cmake --build . --target package
```

### Platform-Specific Outputs

- **Windows**: `build/GlideStop/win.xpl`
- **macOS**: `build/GlideStop/mac.xpl` (Universal Binary)
- **Linux**: `build/GlideStop/lin.xpl`

## Installation

1. Build the plugin following the instructions above
2. Copy the entire `GlideStop` folder to your X-Plane `Resources/plugins/` directory
3. Restart X-Plane

### Directory Structure
```
X-Plane 12/
└── Resources/
    └── plugins/
        └── GlideStop/
            └── win.xpl (or mac.xpl/lin.xpl)
```

## Usage

### Basic Operation

1. **Enable Plugin**: Use the plugin menu (`Plugins > GlideStop > Enable GlideStop`)

2. **Select Wake Category**: Choose appropriate category for your aircraft:
   - Light: Small GA aircraft, light twins
   - Medium: Regional jets, A320 family
   - Heavy: Large transport jets (B747, B777, etc.)
   - Super: A380-class aircraft

3. **Automatic Operation**: Once enabled, GlideStop automatically controls brakes:
   - Use rudder pedals to apply differential braking
   - Pull back on yoke/stick to apply both brakes
   - Brake effectiveness reduces with airspeed
   - Brakes maintained when throttle at idle

### Menu System

The plugin menu provides:
- **Enable/Disable Toggle**: Turn GlideStop on/off
- **Reload Configuration**: Manually reload settings for current aircraft
- **Wake Category Selection**: Choose from Light/Medium/Heavy/Super categories

### Configuration Files

Configuration files are automatically created in each aircraft's directory as `GlideStop.cfg`:

```ini
# GlideStop Configuration File
# Automatic brake control for aircraft without toe braking hardware
#
# Wake Categories:
# 0 = Light (≤7,000 kg) - Rotation speed: 65 kt
# 1 = Medium (7,000-136,000 kg) - Rotation speed: 130 kt
# 2 = Heavy (136,000+ kg) - Rotation speed: 155 kt
# 3 = Super (A380-class) - Rotation speed: 165 kt

enabled=false
wake_category=1
```

## Brake Control Logic

### Input Processing
- **Rudder Input**: Controls differential braking (left/right)
- **Stick Input**: Controls symmetric braking (both wheels)
- **Combined Input**: Uses maximum value when both inputs active

### Effectiveness Calculation
```cpp
float airspeed_factor = max(0.0f, (rotation_speed - current_airspeed) / rotation_speed);
left_brake = clamp(airspeed_factor * (pitch_input + (-yaw_input)), 0.0f, 1.0f);
right_brake = clamp(airspeed_factor * (pitch_input + yaw_input), 0.0f, 1.0f);
```

### Safety Features
- Brake override automatically managed
- Brakes released when plugin disabled
- Realistic airspeed-based effectiveness prevents high-speed braking
- Throttle-idle detection maintains brake pressure during taxi

## Troubleshooting

### Common Issues

1. **Plugin Not Loading**
   - Check Log.txt for error messages
   - Verify SDK installation and platform-specific libraries
   - Ensure plugin is in correct directory structure

2. **Brakes Not Working**
   - Verify plugin is enabled in menu
   - Check that aircraft wake category is appropriate
   - Review Log.txt for dataref errors
   - Ensure no conflicts with other brake-related plugins

3. **Configuration Not Saving**
   - Check aircraft directory permissions
   - Verify aircraft path detection is working
   - Look for filesystem errors in Log.txt

### Debug Information

The plugin writes detailed logging to X-Plane's Log.txt file with prefix "GlideStop:". Common log messages include:

- Plugin initialization status
- Dataref discovery results
- Configuration loading/saving operations
- Wake category changes
- Enable/disable state changes

## Known Limitations

- Rudder input dataref needs to be identified and implemented
- Current implementation uses pitch input for demonstration
- Brake effectiveness calculation is simplified
- No visual feedback for brake pressure in cockpit

## Requirements

- X-Plane 12
- Compatible flight controls (rudder pedals recommended)
- Windows 10+, macOS 10.14+, or Linux with X11

## License

This project follows the same licensing as the TrimGear reference project.