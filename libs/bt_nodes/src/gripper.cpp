#include "bt_nodes/gripper.hpp"
#include "world_sim/world_sim.hpp"
#include <cassert>

static WorldState* get_world(const BT::TreeNode& node) {
    auto* ws = node.config().blackboard->get<WorldState*>("world_state");
    assert(ws && "world_state not set on blackboard");
    return ws;
}

namespace gripper {

BT::NodeStatus OpenAction::onStart()  { return BT::NodeStatus::RUNNING; }
BT::NodeStatus OpenAction::onRunning() {
    WorldSim::open_gripper(*get_world(*this));
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus CloseAction::onStart()  { return BT::NodeStatus::RUNNING; }
BT::NodeStatus CloseAction::onRunning() {
    WorldSim::close_gripper(*get_world(*this));
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus OpenCondition::tick() {
    auto* ws = get_world(*this);
    return ws->gripper == WorldState::GripperState::Open
        ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
}

} // namespace gripper
