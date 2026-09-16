#include "aetherion/core/error.hpp"

#include <gtest/gtest.h>

using aetherion::core::Error;
using aetherion::core::ErrorCode;
using aetherion::core::Result;

TEST(Result, HoldsEitherValidatedValueOrStructuredError) {
    Result<int> success{42};
    ASSERT_TRUE(success);
    EXPECT_EQ(success.value(), 42);

    Result<int> failure{Error{ErrorCode::invalid_argument, "invalid"}};
    ASSERT_FALSE(failure);
    EXPECT_EQ(failure.error().code, ErrorCode::invalid_argument);
}
