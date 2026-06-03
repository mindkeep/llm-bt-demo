#include "bt_nodes/conditions.hpp"
#include "world_sim/world_sim.hpp"

static WorldState* get_world(const BT::TreeNode& node) {
    return node.config().blackboard->get<WorldState*>("world_state");
}

BT::NodeStatus IsGripperOpen::tick() {
    auto* ws = get_world(*this);
    return ws->gripper == WorldState::GripperState::Open
        ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
}

BT::NodeStatus IsObjectAt::tick() {
    auto object_name = getInput<std::string>("object").value();
    auto location_str = getInput<std::string>("location").value();
    auto* ws = get_world(*this);
    auto it = ws->objects.find(object_name);
    if (it == ws->objects.end()) return BT::NodeStatus::FAILURE;
    return it->second.location == location_from_string(location_str)
        ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
}

BT::NodeStatus IsArmNear::tick() {
    auto location_str = getInput<std::string>("location").value();
    auto* ws = get_world(*this);
    return ws->arm_position == location_from_string(location_str)
        ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
}
