#include <gtest/gtest.h>
#include <cstdlib>
#include "behaviortree_cpp/bt_factory.h"
#include "bt_nodes/registry.hpp"
#include "llm_client/llm_client.hpp"
#include "llm_client/bt_xml_validator.hpp"
#include "llm_client/bt_xml_repair_agent.hpp"
#include "llm_client/errors.hpp"

class LLMIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        if (std::getenv("OPENAI_BASE_URL") == nullptr) {
            GTEST_SKIP() << "OPENAI_BASE_URL not set — skipping live LLM tests";
        }
        register_all_nodes(factory);
    }
    BT::BehaviorTreeFactory factory;
    LLMClient llm;
};

TEST_F(LLMIntegrationTest, SimplePickAndPlaceProducesValidXML) {
    BTXMLRepairAgent agent(llm, factory);
    auto xml = agent.get_valid_xml("Pick up object A and move it to location C");
    auto errors = validate_bt_xml(xml, factory);
    EXPECT_TRUE(errors.empty()) << "Validation errors: " << (errors.empty() ? "" : errors.front());
}

TEST_F(LLMIntegrationTest, StackingGoalProducesValidXML) {
    BTXMLRepairAgent agent(llm, factory);
    auto xml = agent.get_valid_xml("Move object B to TableA then move object C to TableB");
    auto errors = validate_bt_xml(xml, factory);
    EXPECT_TRUE(errors.empty()) << "Validation errors: " << (errors.empty() ? "" : errors.front());
}

TEST_F(LLMIntegrationTest, GeneratedXMLIsLoadableIntoFactory) {
    BTXMLRepairAgent agent(llm, factory);
    auto xml = agent.get_valid_xml("Pick up object A and place it at TableB");
    EXPECT_NO_THROW({ auto tree = factory.createTreeFromText(xml); (void)tree; });
}
