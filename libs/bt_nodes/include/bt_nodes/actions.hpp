#pragma once
#include "behaviortree_cpp/action_node.h"
#include "world_sim/world_state.hpp"

class MoveArmTo : public BT::StatefulActionNode {
public:
    MoveArmTo(const std::string& name, const BT::NodeConfig& config)
        : BT::StatefulActionNode(name, config) {}
    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override {}
    static BT::PortsList providedPorts() {
        return { BT::InputPort<std::string>("location") };
    }
};

class OpenGripper : public BT::StatefulActionNode {
public:
    OpenGripper(const std::string& name, const BT::NodeConfig& config)
        : BT::StatefulActionNode(name, config) {}
    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override {}
    static BT::PortsList providedPorts() { return {}; }
};

class CloseGripper : public BT::StatefulActionNode {
public:
    CloseGripper(const std::string& name, const BT::NodeConfig& config)
        : BT::StatefulActionNode(name, config) {}
    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override {}
    static BT::PortsList providedPorts() { return {}; }
};

class PickObject : public BT::StatefulActionNode {
public:
    PickObject(const std::string& name, const BT::NodeConfig& config)
        : BT::StatefulActionNode(name, config) {}
    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override {}
    static BT::PortsList providedPorts() {
        return { BT::InputPort<std::string>("object") };
    }
};

class PlaceObject : public BT::StatefulActionNode {
public:
    PlaceObject(const std::string& name, const BT::NodeConfig& config)
        : BT::StatefulActionNode(name, config) {}
    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override {}
    static BT::PortsList providedPorts() {
        return { BT::InputPort<std::string>("object"),
                 BT::InputPort<std::string>("location") };
    }
};
