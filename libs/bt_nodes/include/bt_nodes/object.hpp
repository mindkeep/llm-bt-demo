#pragma once
#include "behaviortree_cpp/action_node.h"
#include "behaviortree_cpp/condition_node.h"
#include "world_sim/world_state.hpp"

namespace object {

struct PickAction : BT::StatefulActionNode {
    using BT::StatefulActionNode::StatefulActionNode;
    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override {}
    static BT::PortsList providedPorts() {
        return { BT::InputPort<std::string>("object") };
    }
};

struct PlaceAction : BT::StatefulActionNode {
    using BT::StatefulActionNode::StatefulActionNode;
    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override {}
    static BT::PortsList providedPorts() {
        return { BT::InputPort<std::string>("object"),
                 BT::InputPort<std::string>("location") };
    }
};

struct AtCondition : BT::ConditionNode {
    using BT::ConditionNode::ConditionNode;
    BT::NodeStatus tick() override;
    static BT::PortsList providedPorts() {
        return { BT::InputPort<std::string>("object"),
                 BT::InputPort<std::string>("location") };
    }
};

} // namespace object
