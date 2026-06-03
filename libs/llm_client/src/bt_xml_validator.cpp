#include "llm_client/bt_xml_validator.hpp"
#include <stdexcept>

std::vector<std::string> BTXMLValidator::validate(
    const std::string& xml,
    BT::BehaviorTreeFactory& factory) const
{
    std::vector<std::string> errors;
    try {
        (void)factory.createTreeFromText(xml);
    } catch (const std::exception& e) {
        errors.push_back(e.what());
    }
    return errors;
}
