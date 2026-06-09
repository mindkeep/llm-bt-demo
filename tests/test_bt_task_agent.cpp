#include <gtest/gtest.h>
#include "bt_task_agent/bt_task_agent.hpp"
#include "llm_client/llm_client.hpp"
#include "llm_client/bt_xml_repair_agent.hpp"
#include "bt_nodes/registry.hpp"

// XML that executes successfully: move to TableA, pick ObjectA, move to TableB, place it.
static constexpr const char* GOOD_XML = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="Main">
    <Sequence>
      <MoveArmTo location="TableA"/>
      <PickObject object="ObjectA"/>
      <MoveArmTo location="TableB"/>
      <PlaceObject object="ObjectA" location="TableB"/>
    </Sequence>
  </BehaviorTree>
</root>)";

// XML with an unregistered node: triggers BTXMLParseError after BTXMLRepairAgent retries.
static constexpr const char* UNKNOWN_NODE_XML = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="Main">
    <Sequence>
      <If condition="IsObjectAt object=ObjectA location=TableA">
        <MoveArmTo location="TableA"/>
      </If>
    </Sequence>
  </BehaviorTree>
</root>)";

// XML that throws at runtime: PlaceObject on an object that is not held.
static constexpr const char* THROW_XML = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="Main">
    <Sequence>
      <PlaceObject object="ObjectA" location="TableB"/>
    </Sequence>
  </BehaviorTree>
</root>)";

class FakeLLMClient : public LLMClient {
public:
    std::vector<std::string> responses;
    int call_count = 0;
    std::string complete(const std::string&) override {
        return responses.at(call_count++);
    }
};

static BT::BehaviorTreeFactory make_factory() {
    BT::BehaviorTreeFactory f;
    register_all_nodes(f);
    return f;
}

static WorldState make_world() {
    WorldState ws;
    ws.objects["ObjectA"] = {.name = "ObjectA", .location = WorldState::Location::TableA};
    ws.objects["ObjectB"] = {.name = "ObjectB", .location = WorldState::Location::TableB};
    ws.objects["ObjectC"] = {.name = "ObjectC", .location = WorldState::Location::TableC};
    return ws;
}

class BTTaskAgentTest : public ::testing::Test {
protected:
    BT::BehaviorTreeFactory factory = make_factory();
    FakeLLMClient fake_llm;
    // BTXMLRepairAgent and BTTaskAgent created per-test since they hold references
};

TEST_F(BTTaskAgentTest, SucceedsOnFirstAttempt) {
    fake_llm.responses = {GOOD_XML};
    BTXMLRepairAgent repair(fake_llm, factory);
    BTTaskAgent agent(repair, factory, /*max_retries=*/3, /*groot2_port=*/0);

    WorldState world = make_world();
    EXPECT_NO_THROW(agent.execute_goal("move ObjectA to TableB", world));
    EXPECT_EQ(fake_llm.call_count, 1);
    EXPECT_EQ(world.objects.at("ObjectA").location, WorldState::Location::TableB);
}

TEST_F(BTTaskAgentTest, RetriesOnRuntimeErrorAndSucceeds) {
    // First XML throws at runtime; second XML succeeds.
    fake_llm.responses = {THROW_XML, GOOD_XML};
    BTXMLRepairAgent repair(fake_llm, factory);
    BTTaskAgent agent(repair, factory, /*max_retries=*/3, /*groot2_port=*/0);

    WorldState world = make_world();
    EXPECT_NO_THROW(agent.execute_goal("move ObjectA to TableB", world));
    EXPECT_EQ(fake_llm.call_count, 2);
    EXPECT_EQ(world.objects.at("ObjectA").location, WorldState::Location::TableB);
}

TEST_F(BTTaskAgentTest, GivesUpAfterMaxRetries) {
    // All attempts throw; with max_retries=3 the loop runs 4 times total.
    fake_llm.responses = {THROW_XML, THROW_XML, THROW_XML, THROW_XML};
    BTXMLRepairAgent repair(fake_llm, factory);
    BTTaskAgent agent(repair, factory, /*max_retries=*/3, /*groot2_port=*/0);

    WorldState world = make_world();
    EXPECT_THROW(agent.execute_goal("move ObjectA to TableB", world), std::exception);
    EXPECT_EQ(fake_llm.call_count, 4);
}

TEST_F(BTTaskAgentTest, RetriesOnXMLErrorAndSucceeds) {
    // BTXMLRepairAgent(max_retries=1) exhausts on UNKNOWN_NODE_XML and throws
    // BTXMLParseError. BTTaskAgent catches it, feeds the error back to the LLM,
    // and the second attempt returns GOOD_XML.
    fake_llm.responses = {UNKNOWN_NODE_XML, GOOD_XML};
    BTXMLRepairAgent repair(fake_llm, factory, /*max_retries=*/1);
    BTTaskAgent agent(repair, factory, /*max_retries=*/3, /*groot2_port=*/0);

    WorldState world = make_world();
    EXPECT_NO_THROW(agent.execute_goal("rotate all objects", world));
    EXPECT_EQ(fake_llm.call_count, 2);
}

TEST_F(BTTaskAgentTest, GivesUpAfterMaxXMLRetries) {
    // All four attempts (initial + 3 retries) produce unrecognised nodes.
    fake_llm.responses = {UNKNOWN_NODE_XML, UNKNOWN_NODE_XML,
                          UNKNOWN_NODE_XML, UNKNOWN_NODE_XML};
    BTXMLRepairAgent repair(fake_llm, factory, /*max_retries=*/1);
    BTTaskAgent agent(repair, factory, /*max_retries=*/3, /*groot2_port=*/0);

    WorldState world = make_world();
    EXPECT_THROW(agent.execute_goal("rotate all objects", world), std::exception);
    EXPECT_EQ(fake_llm.call_count, 4);
}

TEST_F(BTTaskAgentTest, PropagatesLLMConnectionError) {
    // FakeLLMClient with no responses throws std::out_of_range on access,
    // but we want LLMConnectionError — use a throwing client instead.
    struct ThrowingLLMClient : LLMClient {
        std::string complete(const std::string&) override {
            throw LLMConnectionError("connection refused");
        }
    } throwing_llm;
    BTXMLRepairAgent repair(throwing_llm, factory);
    BTTaskAgent agent(repair, factory, /*max_retries=*/3, /*groot2_port=*/0);

    WorldState world = make_world();
    EXPECT_THROW(agent.execute_goal("move ObjectA to TableB", world), LLMConnectionError);
}
