GlideStop


An XPlane 12 plugin to improve aircraft brakes controlability
when no toe braking hardware exists.


reference:
Wake categories

    J (Super) aircraft types specified as such in Doc 8643 (Aircraft type designators). At present, the only such type is the Airbus A380-800 with a maximum take-off mass in the order of 560 000 kg. (see Airbus A380 Wake Vortex Guidance)
    H (Heavy) aircraft types of 136 000 kg (300 000 lb) or more (except those specified as J);
    M (Medium) aircraft types less than 136 000 kg (300 000 lb) and more than 7 000 kg (15 500 lb); and
    L (Light) aircraft types of 7 000 kg (15 500 lb) or less.

### ROtate speeds
| J (Super) | 150 – 180 kt | 165 kt | 305.58 km/h | 84.88 m/s | e.g. A380-class—very large MTOW; use AFM values. |
| H (Heavy) | 140 – 170 kt | 155 kt | 287.06 km/h | 79.74 m/s | Large transport jets (B747, B777, A330, etc.) — varies with weight. |
| M (Medium) | 110 – 140 kt | 130 kt | 240.76 km/h | 66.88 m/s | Regional jets / smaller airliners (E-Jets, A320 family, etc.). |
| L (Light) | 55 – 75 kt | 65 kt | 120.38 km/h | 33.44 m/s | Small GA/light twins and turboprops. |
----

Within the Plugins/GlideStop plugin menu:
- enabled / disabled toggle
- Radio button for each aircraft wake category




Each aircraft should persist the selected wake category setting in a saved `GlideStop.cfg` file
The config file will be in the current aircarft's directory


# Features
When enabled:
The override_toe_brakes_ref will equal 1
The brakes will be managed using the following systems:

When the user deflects the left rudder, the left brake is also pressed
When the user deflects the right rudder, the right brake is also pressed
When the user pulls back on the stick, both brakes are actuated
** When both rudder and stick inputs are input at the same time, take the largest value
When the throttle is within 1% accuracy of the idle position, the brakes do not release after being pressed
The effectiveness of the breaks diminishes linearly with true airspeed; At rotate speed, brakes cannot be pressed more that 0.0. and when speed is equal to 0.0, brakes are 100% effective.
** See wake categories below on how to determine rotate speed per aircraft

when disabeled:
the override_toe_brakes_ref will equal 0

## Tech
- C++ 17
- Godot C++ 17 coding style conventions
    - Use the std lib wherever possible
- X Plane 12 SDK


FAA Wake Turbulence Categories (for Wake Separation)
This system uses weight-based categories, and some aircraft variants may fall into different categories:

    Category A: A380-800.
    Category B: Upper Heavy Aircraft.
    Category C: Lower Heavy Aircraft.
    Category D: Large Aircraft.
    Category E: Small Plus aircraft.
    Category F: Small Aircraft.



Each aircraft should persist the selected Wake category setting in a saved `TrimGear.cfg` file
The config file will be in the current aircarft's directory
You may have to do some research to find it, but use the SDK's XPLMGetNthAircraftModel(0, FileName, TempPAth). The aircraft at index 0 is the currently loaded aircraft.

** some examples are written in lua, but this project should have 0 lines of lua in it
```lua
dataref("override_toe_brakes_ref", "sim/operation/override/override_toe_brakes", "writable")
dataref("left_brake_ref", "sim/cockpit2/controls/left_brake_ratio", "writable")
dataref("right_brake_ref", "sim/cockpit2/controls/right_brake_ratio", "writable")
dataref("pitch_ref", "sim/joystick/yoke_pitch_ratio", "readonly")
dataref("throttle_ref", "sim/cockpit2/engine/actuators/throttle_ratio_all", "readonly")

// A simpler impl, but a good idea to get started with
			left_brake_ref = math.clamp((-airpseed_factor + (pitch_ref) + (yaw_ref * -1)), 0.0, 1.0)
			right_brake_ref = math.clamp((-airpseed_factor + (pitch_ref) + (yaw_ref * 1)), 0.0, 1.0)


```

true_airspeed_ref = 	sim/cockpit2/gauges/indicators/true_airspeed_kts_pilot
