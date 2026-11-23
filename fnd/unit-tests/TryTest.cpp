#include "fnd/try.h"

#include <gtest/gtest.h>

using namespace HomeCompa::Util;

namespace {

TEST(Try, FailStd)
{
	const auto value = TRY("name", []() -> std::unique_ptr<int> {
		throw std::runtime_error("failed");
	});
	EXPECT_FALSE(!!value);
}

TEST(Try, FailCustom)
{
	const auto value = TRY("name", []() -> std::unique_ptr<int> {
		throw 42;
	});
	EXPECT_FALSE(!!value);
}

TEST(Try, Success)
{
	const auto value = TRY("name", []() -> std::unique_ptr<int> {
		return std::make_unique<int>();
	});
	EXPECT_TRUE(!!value);
}

}
