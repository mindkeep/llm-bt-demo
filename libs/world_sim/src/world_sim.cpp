#include "world_sim/world_sim.hpp"
#include <algorithm>

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
    if (!ws.objects.at(name).held) {
        throw std::logic_error("Cannot place object: not currently held");
    }
    auto& obj = ws.objects.at(name);
    obj.held = false;
    obj.location = location;
}

} // namespace WorldSim
