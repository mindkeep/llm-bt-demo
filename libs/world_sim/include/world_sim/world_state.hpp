#pragma once
#include <array>
#include <string>
#include <unordered_map>

// Plain data model of the simulated world — the single source of truth that BT
// nodes read and WorldSim mutates. No physics, just discrete state: where the
// arm is, whether the gripper is open, and where each object sits.
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
