#include "llm_client/bt_xml_validator.hpp"

std::vector<std::string> validate_bt_xml(const std::string& xml,
                                         BT::BehaviorTreeFactory& factory)
{
    try {
        (void)factory.createTreeFromText(xml);
        return {};
    } catch (const std::exception& e) {
        return { e.what() };
    }
}
