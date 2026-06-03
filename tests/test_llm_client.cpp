#include <gtest/gtest.h>
#include "llm_client/llm_client.hpp"
#include "llm_client/errors.hpp"

TEST(LLMClient, ThrowsOnUnreachableHost) {
    LLMClient client("http://localhost:19999/v1", "", "test-model");
    EXPECT_THROW(client.complete("pick up object A"), LLMConnectionError);
}
