#include <gtest/gtest.h>
#include "world_sim/world_state.hpp"

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
