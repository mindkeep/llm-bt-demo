#include "llm_client/bt_xml_validator.hpp"

// Validation is "does it actually load?" — we build a throwaway tree from the
// XML and let BT.CPP be the judge. This catches malformed XML, unknown node
// names, and bad ports in one shot, and the thrown message is exactly what we
// feed back to the LLM as a repair hint.
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
