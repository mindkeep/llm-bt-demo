#pragma once
#include "behaviortree_cpp/action_node.h"
#include "behaviortree_cpp/condition_node.h"

namespace gripper {

struct OpenAction : BT::StatefulActionNode {
    using BT::StatefulActionNode::StatefulActionNode;
    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override {}
    static BT::PortsList providedPorts() { return {}; }
};

struct CloseAction : BT::StatefulActionNode {
    using BT::StatefulActionNode::StatefulActionNode;
    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override {}
    static BT::PortsList providedPorts() { return {}; }
};

struct OpenCondition : BT::ConditionNode {
    using BT::ConditionNode::ConditionNode;
    BT::NodeStatus tick() override;
    static BT::PortsList providedPorts() { return {}; }
};

} // namespace gripper
