#include <gtest/gtest.h>
#include "behaviortree_cpp/bt_factory.h"
#include "bt_nodes/conditions.hpp"
#include "bt_nodes/registry.hpp"
#include "world_sim/world_state.hpp"

// Helper: build a single-node tree and tick it once
static BT::NodeStatus tick_tree(const std::string& xml, WorldState& ws) {
    BT::BehaviorTreeFactory factory;
    register_all_nodes(factory);
    auto bb = BT::Blackboard::create();
    bb->set("world_state", &ws);
    auto tree = factory.createTreeFromText(xml, bb);
    return tree.tickOnce();
}

static const char* XML_IS_GRIPPER_OPEN = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="T"><IsGripperOpen/></BehaviorTree>
</root>)";

TEST(Conditions, IsGripperOpenSucceedsWhenOpen) {
    WorldState ws;
    ws.gripper = WorldState::GripperState::Open;
    EXPECT_EQ(tick_tree(XML_IS_GRIPPER_OPEN, ws), BT::NodeStatus::SUCCESS);
}

TEST(Conditions, IsGripperOpenFailsWhenClosed) {
    WorldState ws;
    ws.gripper = WorldState::GripperState::Closed;
    EXPECT_EQ(tick_tree(XML_IS_GRIPPER_OPEN, ws), BT::NodeStatus::FAILURE);
}

TEST(Conditions, IsObjectAtSucceeds) {
    WorldState ws;
    ws.objects["ObjectA"] = {"ObjectA", WorldState::Location::TableA, false};
    const char* xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="T">
    <IsObjectAt object="ObjectA" location="TableA"/>
  </BehaviorTree>
</root>)";
    EXPECT_EQ(tick_tree(xml, ws), BT::NodeStatus::SUCCESS);
}

TEST(Conditions, IsObjectAtFailsWrongLocation) {
    WorldState ws;
    ws.objects["ObjectA"] = {"ObjectA", WorldState::Location::TableB, false};
    const char* xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="T">
    <IsObjectAt object="ObjectA" location="TableA"/>
  </BehaviorTree>
</root>)";
    EXPECT_EQ(tick_tree(xml, ws), BT::NodeStatus::FAILURE);
}

TEST(Conditions, IsArmNearSucceeds) {
    WorldState ws;
    ws.arm_position = WorldState::Location::TableC;
    const char* xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="T">
    <IsArmNear location="TableC"/>
  </BehaviorTree>
</root>)";
    EXPECT_EQ(tick_tree(xml, ws), BT::NodeStatus::SUCCESS);
}

TEST(Conditions, IsArmNearFails) {
    WorldState ws;
    ws.arm_position = WorldState::Location::TableA;
    const char* xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="T">
    <IsArmNear location="TableC"/>
  </BehaviorTree>
</root>)";
    EXPECT_EQ(tick_tree(xml, ws), BT::NodeStatus::FAILURE);
}
