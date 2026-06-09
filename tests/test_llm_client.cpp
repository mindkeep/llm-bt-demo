#include <gtest/gtest.h>
#include "llm_client/llm_client.hpp"
#include "llm_client/errors.hpp"
#include "llm_client/bt_xml_validator.hpp"
#include "llm_client/bt_xml_repair_agent.hpp"
#include "bt_nodes/registry.hpp"

static BT::BehaviorTreeFactory make_factory() {
    BT::BehaviorTreeFactory f;
    register_all_nodes(f);
    return f;
}

TEST(LLMClient, ThrowsOnUnreachableHost) {
    LLMClient client("http://localhost:19999/v1", "", "test-model");
    EXPECT_THROW(client.complete("pick up object A"), LLMConnectionError);
}

TEST(ValidateBTXML, ValidXmlReturnsNoErrors) {
    auto factory = make_factory();
    const std::string valid_xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="Main">
    <Sequence>
      <MoveArmTo location="TableA"/>
      <PickObject object="ObjectA"/>
    </Sequence>
  </BehaviorTree>
</root>)";
    auto errors = validate_bt_xml(valid_xml, factory);
    EXPECT_TRUE(errors.empty()) << errors.front();
}

TEST(ValidateBTXML, UnknownNodeNameReturnsError) {
    auto factory = make_factory();
    const std::string bad_xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="Main">
    <GrabObject object="ObjectA"/>
  </BehaviorTree>
</root>)";
    auto errors = validate_bt_xml(bad_xml, factory);
    EXPECT_FALSE(errors.empty());
    EXPECT_NE(errors.front().find("GrabObject"), std::string::npos);
}

TEST(ValidateBTXML, MalformedXmlReturnsError) {
    auto factory = make_factory();
    auto errors = validate_bt_xml("this is not xml at all", factory);
    EXPECT_FALSE(errors.empty());
}

TEST(ValidateBTXML, MissingRootElementReturnsError) {
    auto factory = make_factory();
    auto errors = validate_bt_xml(R"(<MoveArmTo location="TableA"/>)", factory);
    EXPECT_FALSE(errors.empty());
}

static const std::string VALID_XML = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="Main">
    <MoveArmTo location="TableA"/>
  </BehaviorTree>
</root>)";

static const std::string INVALID_XML = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="Main">
    <GrabObject object="ObjectA"/>
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

TEST(BTXMLRepairAgent, ReturnsValidXmlOnFirstAttempt) {
    FakeLLMClient fake;
    fake.responses = {VALID_XML};
    auto factory = make_factory();

    BTXMLRepairAgent agent(fake, factory);
    auto xml = agent.get_valid_xml("pick up A");
    EXPECT_EQ(fake.call_count, 1);
    EXPECT_FALSE(xml.empty());
}

TEST(BTXMLRepairAgent, RetriesOnInvalidXmlAndSucceeds) {
    FakeLLMClient fake;
    fake.responses = {INVALID_XML, VALID_XML};
    auto factory = make_factory();

    BTXMLRepairAgent agent(fake, factory);
    auto xml = agent.get_valid_xml("pick up A");
    EXPECT_EQ(fake.call_count, 2);
    EXPECT_FALSE(xml.empty());
}

TEST(BTXMLRepairAgent, ThrowsAfterMaxRetries) {
    FakeLLMClient fake;
    fake.responses = {INVALID_XML, INVALID_XML, INVALID_XML};
    auto factory = make_factory();

    BTXMLRepairAgent agent(fake, factory, /*max_retries=*/3);
    EXPECT_THROW(agent.get_valid_xml("pick up A"), BTXMLParseError);
    EXPECT_EQ(fake.call_count, 3);
}
