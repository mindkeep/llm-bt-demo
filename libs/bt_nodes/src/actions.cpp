#include "bt_nodes/actions.hpp"
#include "world_sim/world_sim.hpp"
#include <cassert>

static WorldState* get_world(const BT::TreeNode& node) {
    auto* ws = node.config().blackboard->get<WorldState*>("world_state");
    assert(ws && "world_state not set on blackboard");
    return ws;
}

// MoveArmTo
BT::NodeStatus MoveArmTo::onStart() { return BT::NodeStatus::RUNNING; }
BT::NodeStatus MoveArmTo::onRunning() {
    auto res = getInput<std::string>("location");
    if (!res) return BT::NodeStatus::FAILURE;
    WorldSim::move_arm_to(*get_world(*this), location_from_string(res.value()));
    return BT::NodeStatus::SUCCESS;
}

// OpenGripper
BT::NodeStatus OpenGripper::onStart() { return BT::NodeStatus::RUNNING; }
BT::NodeStatus OpenGripper::onRunning() {
    WorldSim::open_gripper(*get_world(*this));
    return BT::NodeStatus::SUCCESS;
}

// CloseGripper
BT::NodeStatus CloseGripper::onStart() { return BT::NodeStatus::RUNNING; }
BT::NodeStatus CloseGripper::onRunning() {
    WorldSim::close_gripper(*get_world(*this));
    return BT::NodeStatus::SUCCESS;
}

// PickObject
BT::NodeStatus PickObject::onStart() { return BT::NodeStatus::RUNNING; }
BT::NodeStatus PickObject::onRunning() {
    auto res = getInput<std::string>("object");
    if (!res) return BT::NodeStatus::FAILURE;
    WorldSim::pick_object(*get_world(*this), res.value());
    return BT::NodeStatus::SUCCESS;
}

// PlaceObject
BT::NodeStatus PlaceObject::onStart() { return BT::NodeStatus::RUNNING; }
BT::NodeStatus PlaceObject::onRunning() {
    auto obj_res = getInput<std::string>("object");
    auto loc_res = getInput<std::string>("location");
    if (!obj_res || !loc_res) return BT::NodeStatus::FAILURE;
    WorldSim::place_object(*get_world(*this), obj_res.value(), location_from_string(loc_res.value()));
    return BT::NodeStatus::SUCCESS;
}
