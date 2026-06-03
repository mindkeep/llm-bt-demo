#pragma once
#include <array>
#include <string>
#include <unordered_map>

struct WorldState {
    enum class GripperState { Open, Closed };
    enum class Location { TableA, TableB, TableC, ArmReach, Unknown };

    struct Object {
        std::string name;
        Location location = Location::Unknown;
        bool held = false;
    };

    GripperState gripper = GripperState::Open;
    Location arm_position = Location::Unknown;
    std::array<float, 6> joint_angles = {};
    std::unordered_map<std::string, Object> objects;
};
