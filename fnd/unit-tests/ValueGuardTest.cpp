#include "fnd/ValueGuard.h"

#include <gtest/gtest.h>

using namespace HomeCompa;

namespace {

TEST(ValueGuard, Int)
{
	auto value = 42;
	{
		const ValueGuard _(value, 17);
		EXPECT_EQ(value, 17);
		value = -5;
		EXPECT_EQ(value, -5);
	}
	EXPECT_EQ(value, 42);
}

TEST(ValueGuard, Double)
{
	auto value = 42.0;
	{
		const ValueGuard _(value, 17.0);
		EXPECT_DOUBLE_EQ(value, 17.0);
		value = -5.0;
		EXPECT_DOUBLE_EQ(value, -5.0);
	}
	EXPECT_EQ(value, 42.0);
}

TEST(ValueGuard, String)
{
	std::string value = "42.0";
	{
		const ValueGuard _(value, std::string { "17.0" });
		EXPECT_STREQ(value.data(), "17.0");
		value = "-5.0";
		EXPECT_STREQ(value.data(), "-5.0");
	}
	EXPECT_STREQ(value.data(), "42.0");
}

TEST(ValueGuard, Enum)
{
	enum class MyEnum
	{
		e1,
		e2,
		e3
	};
	auto value = MyEnum::e1;
	{
		const ValueGuard _(value, MyEnum::e2);
		EXPECT_EQ(value, MyEnum::e2);
		value = MyEnum::e3;
		EXPECT_EQ(value, MyEnum::e3);
	}
	EXPECT_EQ(value, MyEnum::e1);
}

TEST(ValueGuard, Ptr)
{
	const std::vector<int> values { 42, 17, -5 };
	const auto*            value = values.data();
	{
		const ValueGuard _(value, values.data() + 1);
		EXPECT_EQ(*value, 17);
		value = values.data() + 2;
		EXPECT_EQ(*value, -5);
	}
	EXPECT_EQ(*value, 42);
}

} // namespace
