#include "world_sim/world_sim.hpp"
#include <algorithm>

// The only place WorldState is mutated. Each function enforces the physical
// preconditions of the simulated arm (e.g. only one object held at a time) and
// throws std::logic_error when violated. The BT nodes call these, and a thrown
// error surfaces as a runtime failure that BTTaskAgent feeds back to the LLM.
namespace WorldSim {

void move_arm_to(WorldState& ws, WorldState::Location location) {
    ws.arm_position = location;
}

void open_gripper(WorldState& ws) {
    ws.gripper = WorldState::GripperState::Open;
}

void close_gripper(WorldState& ws) {
    ws.gripper = WorldState::GripperState::Closed;
}

void pick_object(WorldState& ws, const std::string& name) {
    bool already_holding = std::ranges::any_of(ws.objects,
        [](const auto& kv) { return kv.second.held; });
    if (already_holding) {
        throw std::logic_error("Cannot pick object: already holding one");
    }
    auto& obj = ws.objects.at(name);
    obj.held = true;
    obj.location = WorldState::Location::ArmReach;
}

void place_object(WorldState& ws, const std::string& name, WorldState::Location location) {
    auto& obj = ws.objects.at(name);
    if (!obj.held) {
        throw std::logic_error("Cannot place object: not currently held");
    }
    obj.held = false;
    obj.location = location;
}

} // namespace WorldSim
