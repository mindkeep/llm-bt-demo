#include "bt_nodes/conditions.hpp"
#include "world_sim/world_sim.hpp"
#include <cassert>

static WorldState* get_world(const BT::TreeNode& node) {
    auto* ws = node.config().blackboard->get<WorldState*>("world_state");
    assert(ws && "world_state not set on blackboard");
    return ws;
}

BT::NodeStatus IsGripperOpen::tick() {
    auto* ws = get_world(*this);
    return ws->gripper == WorldState::GripperState::Open
        ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
}

BT::NodeStatus IsObjectAt::tick() {
    auto object_res = getInput<std::string>("object");
    auto location_res = getInput<std::string>("location");
    if (!object_res || !location_res) return BT::NodeStatus::FAILURE;
    auto object_name = object_res.value();
    auto location_str = location_res.value();
    auto* ws = get_world(*this);
    auto it = ws->objects.find(object_name);
    if (it == ws->objects.end()) return BT::NodeStatus::FAILURE;
    return it->second.location == location_from_string(location_str)
        ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
}

BT::NodeStatus IsArmNear::tick() {
    auto location_res = getInput<std::string>("location");
    if (!location_res) return BT::NodeStatus::FAILURE;
    auto location_str = location_res.value();
    auto* ws = get_world(*this);
    return ws->arm_position == location_from_string(location_str)
        ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
}
