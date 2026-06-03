#include <gtest/gtest.h>
#include "llm_client/llm_client.hpp"
#include "llm_client/errors.hpp"
#include "llm_client/bt_xml_validator.hpp"
#include "bt_nodes/registry.hpp"

TEST(LLMClient, ThrowsOnUnreachableHost) {
    LLMClient client("http://localhost:19999/v1", "", "test-model");
    EXPECT_THROW(client.complete("pick up object A"), LLMConnectionError);
}

static BT::BehaviorTreeFactory make_factory() {
    BT::BehaviorTreeFactory f;
    register_all_nodes(f);
    return f;
}

TEST(BTXMLValidator, ValidXmlReturnsNoErrors) {
    auto factory = make_factory();
    BTXMLValidator validator;
    const std::string valid_xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="Main">
    <Sequence>
      <MoveArmTo location="TableA"/>
      <PickObject object="ObjectA"/>
    </Sequence>
  </BehaviorTree>
</root>)";
    auto errors = validator.validate(valid_xml, factory);
    EXPECT_TRUE(errors.empty()) << errors.front();
}

TEST(BTXMLValidator, UnknownNodeNameReturnsError) {
    auto factory = make_factory();
    BTXMLValidator validator;
    const std::string bad_xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="Main">
    <GrabObject object="ObjectA"/>
  </BehaviorTree>
</root>)";
    auto errors = validator.validate(bad_xml, factory);
    EXPECT_FALSE(errors.empty());
    EXPECT_NE(errors.front().find("GrabObject"), std::string::npos);
}

TEST(BTXMLValidator, MalformedXmlReturnsError) {
    auto factory = make_factory();
    BTXMLValidator validator;
    const std::string not_xml = "this is not xml at all";
    auto errors = validator.validate(not_xml, factory);
    EXPECT_FALSE(errors.empty());
}

TEST(BTXMLValidator, MissingRootElementReturnsError) {
    auto factory = make_factory();
    BTXMLValidator validator;
    const std::string no_root = R"(<MoveArmTo location="TableA"/>)";
    auto errors = validator.validate(no_root, factory);
    EXPECT_FALSE(errors.empty());
}
