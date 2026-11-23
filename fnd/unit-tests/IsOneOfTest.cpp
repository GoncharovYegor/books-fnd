#include "fnd/IsOneOf.h"

#include <gtest/gtest.h>

#include <QString>

using namespace HomeCompa;

namespace {

TEST(IsOneOf, Int)
{
	EXPECT_FALSE(IsOneOf(0));
	EXPECT_TRUE(IsOneOf(0, 0));
	EXPECT_TRUE(IsOneOf(0, 0, 1));
	EXPECT_TRUE(IsOneOf(0, 0, 1, 42));
	EXPECT_TRUE(IsOneOf(42, 0, 1, 42, 18));
	EXPECT_FALSE(IsOneOf(43, 0, 1, 42, 18));
}

TEST(IsOneOf, StdString)
{
	const std::string test { "test" };
	EXPECT_FALSE(IsOneOf(test));
	EXPECT_TRUE(IsOneOf(test, "test"));
	EXPECT_TRUE(IsOneOf(test, "", "test", "1"));
	EXPECT_FALSE(IsOneOf(test, "", "test1", "1"));
}

TEST(IsOneOf, QString)
{
	const QString test { "test" };
	EXPECT_FALSE(IsOneOf(test));
	EXPECT_TRUE(IsOneOf(test, "test"));
	EXPECT_TRUE(IsOneOf(test, "", "test", "1"));
	EXPECT_FALSE(IsOneOf(test, "", "test1", "1"));
}

enum class MyEnum
{
	Value0,
	Value1,
	Value2,
	Value3,
	Last
};

TEST(IsOneOf, Enum)
{
	const auto test = MyEnum::Value1;
	EXPECT_FALSE(IsOneOf(test));
	EXPECT_TRUE(IsOneOf(test, MyEnum::Value1));
	EXPECT_TRUE(IsOneOf(test, MyEnum::Value0, MyEnum::Value1, MyEnum::Value2, MyEnum::Value1));
	EXPECT_FALSE(IsOneOf(test, MyEnum::Value0));
	EXPECT_FALSE(IsOneOf(test, MyEnum::Last, MyEnum::Value0, MyEnum::Value2, MyEnum::Value3));
}

}
