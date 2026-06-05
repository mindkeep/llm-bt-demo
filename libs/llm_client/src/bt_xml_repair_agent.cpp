#include "llm_client/bt_xml_repair_agent.hpp"

BTXMLRepairAgent::BTXMLRepairAgent(LLMClient& client,
                                   BTXMLValidator& validator,
                                   BT::BehaviorTreeFactory& factory,
                                   int max_retries)
    : client_(client), validator_(validator), factory_(factory), max_retries_(max_retries)
{}

std::string BTXMLRepairAgent::repair_prompt(
    const std::string& goal,
    const std::string& bad_xml,
    std::span<const std::string> errors) const
{
    std::string prompt = "Goal: " + goal + "\n\n";
    prompt += "Your previous output was invalid. Errors:\n";
    for (const auto& e : errors) {
        prompt += "- " + e + "\n";
    }
    prompt += "\nPrevious invalid XML:\n" + bad_xml + "\n\nCorrected XML only:";
    return prompt;
}

std::string BTXMLRepairAgent::get_valid_xml(const std::string& goal) {
    std::string last_xml;
    std::vector<std::string> last_errors;

    for (int attempt = 0; attempt < max_retries_; ++attempt) {
        const std::string user_msg = (attempt == 0)
            ? "Goal: " + goal
            : repair_prompt(goal, last_xml, last_errors);

        last_xml = client_.complete(user_msg);
        last_errors = validator_.validate(last_xml, factory_);

        if (last_errors.empty()) {
            return last_xml;
        }
    }

    std::string error_summary;
    for (const auto& e : last_errors) {
        if (!error_summary.empty()) error_summary += "; ";
        error_summary += e;
    }
    throw BTXMLParseError(error_summary, last_xml);
}
