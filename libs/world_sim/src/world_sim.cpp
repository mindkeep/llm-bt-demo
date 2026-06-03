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
    bool already_holding = std::any_of(ws.objects.begin(), ws.objects.end(),
        [](const auto& kv) { return kv.second.held; });
    if (already_holding) {
        throw std::logic_error("Cannot pick object: already holding one");
    }
    ws.objects.at(name).held = true;
    ws.objects.at(name).location = WorldState::Location::ArmReach;
}

void place_object(WorldState& ws, const std::string& name, WorldState::Location location) {
    if (!ws.objects.at(name).held) {
        throw std::logic_error("Cannot place object: not currently held");
    }
    ws.objects.at(name).held = false;
    ws.objects.at(name).location = location;
}

} // namespace WorldSim
