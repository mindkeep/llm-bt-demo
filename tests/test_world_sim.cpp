#include <gtest/gtest.h>
#include "world_sim/world_state.hpp"
#include "world_sim/world_sim.hpp"

TEST(WorldState, DefaultGripperIsOpen) {
    WorldState ws;
    EXPECT_EQ(ws.gripper, WorldState::GripperState::Open);
}

TEST(WorldState, DefaultArmPositionIsUnknown) {
    WorldState ws;
    EXPECT_EQ(ws.arm_position, WorldState::Location::Unknown);
}

TEST(WorldState, DefaultJointAnglesAreZero) {
    WorldState ws;
    for (float angle : ws.joint_angles) {
        EXPECT_FLOAT_EQ(angle, 0.0f);
    }
}

TEST(WorldState, ObjectsMapIsEmptyByDefault) {
    WorldState ws;
    EXPECT_TRUE(ws.objects.empty());
}

TEST(WorldState, ObjectCanBeAddedAndRetrieved) {
    WorldState ws;
    ws.objects["ObjectA"] = {"ObjectA", WorldState::Location::TableA, false};
    ASSERT_TRUE(ws.objects.count("ObjectA"));
    EXPECT_EQ(ws.objects.at("ObjectA").location, WorldState::Location::TableA);
    EXPECT_FALSE(ws.objects.at("ObjectA").held);
}

TEST(WorldSim, MoveArmUpdatesPosition) {
    WorldState ws;
    WorldSim::move_arm_to(ws, WorldState::Location::TableA);
    EXPECT_EQ(ws.arm_position, WorldState::Location::TableA);
}

TEST(WorldSim, OpenGripperSetsState) {
    WorldState ws;
    ws.gripper = WorldState::GripperState::Closed;
    WorldSim::open_gripper(ws);
    EXPECT_EQ(ws.gripper, WorldState::GripperState::Open);
}

TEST(WorldSim, CloseGripperSetsState) {
    WorldState ws;
    WorldSim::close_gripper(ws);
    EXPECT_EQ(ws.gripper, WorldState::GripperState::Closed);
}

TEST(WorldSim, PickObjectSetsHeldAndLocation) {
    WorldState ws;
    ws.objects["ObjectA"] = {"ObjectA", WorldState::Location::TableA, false};
    ws.arm_position = WorldState::Location::TableA;
    ws.gripper = WorldState::GripperState::Open;

    WorldSim::pick_object(ws, "ObjectA");

    EXPECT_TRUE(ws.objects.at("ObjectA").held);
    EXPECT_EQ(ws.objects.at("ObjectA").location, WorldState::Location::ArmReach);
}

TEST(WorldSim, PickObjectThrowsIfAlreadyHoldingOne) {
    WorldState ws;
    ws.objects["ObjectA"] = {"ObjectA", WorldState::Location::TableA, true};
    ws.objects["ObjectB"] = {"ObjectB", WorldState::Location::TableA, false};

    EXPECT_THROW(WorldSim::pick_object(ws, "ObjectB"), std::logic_error);
}

TEST(WorldSim, PlaceObjectUpdatesLocationAndClearsHeld) {
    WorldState ws;
    ws.objects["ObjectA"] = {"ObjectA", WorldState::Location::ArmReach, true};

    WorldSim::place_object(ws, "ObjectA", WorldState::Location::TableC);

    EXPECT_FALSE(ws.objects.at("ObjectA").held);
    EXPECT_EQ(ws.objects.at("ObjectA").location, WorldState::Location::TableC);
}

TEST(WorldSim, PlaceObjectThrowsIfNotHeld) {
    WorldState ws;
    ws.objects["ObjectA"] = {"ObjectA", WorldState::Location::TableA, false};

    EXPECT_THROW(WorldSim::place_object(ws, "ObjectA", WorldState::Location::TableC), std::logic_error);
}

TEST(WorldSim, LocationFromStringMapsAllValues) {
    EXPECT_EQ(location_from_string("TableA"),   WorldState::Location::TableA);
    EXPECT_EQ(location_from_string("TableB"),   WorldState::Location::TableB);
    EXPECT_EQ(location_from_string("TableC"),   WorldState::Location::TableC);
    EXPECT_EQ(location_from_string("ArmReach"), WorldState::Location::ArmReach);
    EXPECT_EQ(location_from_string("garbage"),  WorldState::Location::Unknown);
    EXPECT_EQ(location_from_string(""),         WorldState::Location::Unknown);
}
