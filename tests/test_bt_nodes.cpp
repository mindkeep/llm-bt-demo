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
    ws.objects["ObjectA"] = {.name = "ObjectA", .location = WorldState::Location::TableA};
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
    ws.objects["ObjectA"] = {.name = "ObjectA", .location = WorldState::Location::TableB};
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

TEST(Actions, MoveArmToReturnsRunningThenSuccess) {
    WorldState ws;
    const char* xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="T">
    <MoveArmTo location="TableB"/>
  </BehaviorTree>
</root>)";
    BT::BehaviorTreeFactory factory;
    register_all_nodes(factory);
    auto bb = BT::Blackboard::create();
    bb->set("world_state", &ws);
    auto tree = factory.createTreeFromText(xml, bb);

    EXPECT_EQ(tree.tickOnce(), BT::NodeStatus::RUNNING);
    EXPECT_EQ(tree.tickOnce(), BT::NodeStatus::SUCCESS);
    EXPECT_EQ(ws.arm_position, WorldState::Location::TableB);
}

TEST(Actions, OpenGripperSucceeds) {
    WorldState ws;
    ws.gripper = WorldState::GripperState::Closed;
    const char* xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="T"><OpenGripper/></BehaviorTree>
</root>)";
    BT::BehaviorTreeFactory factory;
    register_all_nodes(factory);
    auto bb = BT::Blackboard::create();
    bb->set("world_state", &ws);
    auto tree = factory.createTreeFromText(xml, bb);

    tree.tickOnce(); tree.tickOnce();
    EXPECT_EQ(ws.gripper, WorldState::GripperState::Open);
}

TEST(Actions, CloseGripperSucceeds) {
    WorldState ws;
    const char* xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="T"><CloseGripper/></BehaviorTree>
</root>)";
    BT::BehaviorTreeFactory factory;
    register_all_nodes(factory);
    auto bb = BT::Blackboard::create();
    bb->set("world_state", &ws);
    auto tree = factory.createTreeFromText(xml, bb);

    tree.tickOnce(); tree.tickOnce();
    EXPECT_EQ(ws.gripper, WorldState::GripperState::Closed);
}

TEST(Actions, PickObjectSetsHeld) {
    WorldState ws;
    ws.objects["ObjectA"] = {.name = "ObjectA", .location = WorldState::Location::TableA};
    const char* xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="T">
    <PickObject object="ObjectA"/>
  </BehaviorTree>
</root>)";
    BT::BehaviorTreeFactory factory;
    register_all_nodes(factory);
    auto bb = BT::Blackboard::create();
    bb->set("world_state", &ws);
    auto tree = factory.createTreeFromText(xml, bb);

    tree.tickOnce(); tree.tickOnce();
    EXPECT_TRUE(ws.objects.at("ObjectA").held);
}

TEST(Actions, PlaceObjectUpdatesLocation) {
    WorldState ws;
    ws.objects["ObjectA"] = {.name = "ObjectA", .location = WorldState::Location::ArmReach, .held = true};
    const char* xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="T">
    <PlaceObject object="ObjectA" location="TableC"/>
  </BehaviorTree>
</root>)";
    BT::BehaviorTreeFactory factory;
    register_all_nodes(factory);
    auto bb = BT::Blackboard::create();
    bb->set("world_state", &ws);
    auto tree = factory.createTreeFromText(xml, bb);

    tree.tickOnce(); tree.tickOnce();
    EXPECT_EQ(ws.objects.at("ObjectA").location, WorldState::Location::TableC);
    EXPECT_FALSE(ws.objects.at("ObjectA").held);
}
