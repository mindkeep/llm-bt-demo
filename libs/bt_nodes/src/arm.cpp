#include "bt_nodes/arm.hpp"
#include "world_sim/world_sim.hpp"
#include <cassert>

static WorldState* get_world(const BT::TreeNode& node) {
    auto* ws = node.config().blackboard->get<WorldState*>("world_state");
    assert(ws && "world_state not set on blackboard");
    return ws;
}

namespace arm {

BT::NodeStatus MoveAction::onStart() { return BT::NodeStatus::RUNNING; }
BT::NodeStatus MoveAction::onRunning() {
    auto res = getInput<std::string>("location");
    if (!res) return BT::NodeStatus::FAILURE;
    WorldSim::move_arm_to(*get_world(*this), location_from_string(res.value()));
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus NearCondition::tick() {
    auto res = getInput<std::string>("location");
    if (!res) return BT::NodeStatus::FAILURE;
    auto* ws = get_world(*this);
    return ws->arm_position == location_from_string(res.value())
        ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
}

} // namespace arm
