#pragma once
#include <stdexcept>
#include <string>
#include <string_view>
#include "world_sim/world_state.hpp"

namespace WorldSim {
    void move_arm_to(WorldState& ws, WorldState::Location location);
    void open_gripper(WorldState& ws);
    void close_gripper(WorldState& ws);
    void pick_object(WorldState& ws, const std::string& object_name);
    void place_object(WorldState& ws, const std::string& object_name, WorldState::Location location);
}

inline WorldState::Location location_from_string(std::string_view s) {
    if (s == "TableA")   return WorldState::Location::TableA;
    if (s == "TableB")   return WorldState::Location::TableB;
    if (s == "TableC")   return WorldState::Location::TableC;
    if (s == "ArmReach") return WorldState::Location::ArmReach;
    return WorldState::Location::Unknown;
}

inline std::string_view location_to_string(WorldState::Location loc) {
    switch (loc) {
        case WorldState::Location::TableA:   return "TableA";
        case WorldState::Location::TableB:   return "TableB";
        case WorldState::Location::TableC:   return "TableC";
        case WorldState::Location::ArmReach: return "ArmReach";
        default:                             return "Unknown";
    }
}
