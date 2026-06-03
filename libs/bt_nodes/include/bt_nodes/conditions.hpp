#pragma once
#include "behaviortree_cpp/condition_node.h"
#include "world_sim/world_state.hpp"

class IsGripperOpen : public BT::ConditionNode {
public:
    IsGripperOpen(const std::string& name, const BT::NodeConfig& config)
        : BT::ConditionNode(name, config) {}
    BT::NodeStatus tick() override;
    static BT::PortsList providedPorts() { return {}; }
};

class IsObjectAt : public BT::ConditionNode {
public:
    IsObjectAt(const std::string& name, const BT::NodeConfig& config)
        : BT::ConditionNode(name, config) {}
    BT::NodeStatus tick() override;
    static BT::PortsList providedPorts() {
        return { BT::InputPort<std::string>("object"),
                 BT::InputPort<std::string>("location") };
    }
};

class IsArmNear : public BT::ConditionNode {
public:
    IsArmNear(const std::string& name, const BT::NodeConfig& config)
        : BT::ConditionNode(name, config) {}
    BT::NodeStatus tick() override;
    static BT::PortsList providedPorts() {
        return { BT::InputPort<std::string>("location") };
    }
};
