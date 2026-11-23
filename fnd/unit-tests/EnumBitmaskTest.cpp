#include "fnd/EnumBitmask.h"

#include <gtest/gtest.h>

namespace {

enum MyEnum1
{
	Value0 = 1 << 0,
	Value1 = 1 << 1,
};

enum MyEnum2
{
	Value2 = 1 << 2,
	Value3 = 1 << 3,
};

#if defined(__clang__)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wenum-compare"
	#if __has_warning("-Wdeprecated-enum-enum-conversion")
		#pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"
	#endif
#elif defined(__GNUC__)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wenum-compare"
	#if __GNUC__ >= 11
		#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
	#endif
#elif defined(_MSC_VER)
	#pragma warning(push)
	#pragma warning(disable : 5054) // C++20: operator between different enumerations
	#pragma warning(disable : 4805) // unsafe mix of type 'type' and type 'type' in operation
#endif

TEST(BadEnums, Fail)
{
	EXPECT_NE(MyEnum1::Value0 | MyEnum2::Value3, 0);
	EXPECT_EQ(MyEnum1::Value0 & MyEnum2::Value3, 0);
}

#if defined(__clang__)
	#pragma clang diagnostic pop
#elif defined(__GNUC__)
	#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
	#pragma warning(pop)
#endif

} // namespace

enum class MyEnumClass
{
	None   = 0,
	Value0 = 1 << 0,
	Value1 = 1 << 1,
	All    = Value0 | Value1
};

ENABLE_BITMASK_OPERATORS(MyEnumClass);

namespace {

TEST(EnumBitmask, Success)
{
	EXPECT_EQ((~MyEnumClass::Value0 & MyEnumClass::Value0), MyEnumClass::None);
	EXPECT_EQ((~MyEnumClass::Value0 | MyEnumClass::Value0), ~MyEnumClass::None);
	EXPECT_EQ((MyEnumClass::Value0 | MyEnumClass::Value0), MyEnumClass::Value0);
	EXPECT_EQ((MyEnumClass::Value0 & MyEnumClass::Value0), MyEnumClass::Value0);
	EXPECT_EQ((MyEnumClass::Value0 | MyEnumClass::Value1), MyEnumClass::All);
	EXPECT_EQ((MyEnumClass::Value0 & MyEnumClass::Value1), MyEnumClass::None);
	EXPECT_TRUE(!MyEnumClass::None);
	EXPECT_FALSE(!!MyEnumClass::None);
	EXPECT_TRUE(!!MyEnumClass::Value0);
	EXPECT_FALSE(!MyEnumClass::Value0);
	EXPECT_TRUE(!!(MyEnumClass::All & MyEnumClass::Value0));
	EXPECT_FALSE(!!(MyEnumClass::None & MyEnumClass::Value0));
	EXPECT_TRUE(!!(MyEnumClass::All | MyEnumClass::Value0));
	EXPECT_TRUE(!!(MyEnumClass::None | MyEnumClass::Value0));

	auto value = MyEnumClass::None;
	EXPECT_TRUE(!!(value | MyEnumClass::Value0));
	EXPECT_FALSE(!!(value & MyEnumClass::Value0));
	EXPECT_TRUE(!!(value | MyEnumClass::Value1));
	EXPECT_FALSE(!!(value & MyEnumClass::Value1));

	EXPECT_NO_THROW(value |= MyEnumClass::Value0);
	EXPECT_TRUE(!!(value | MyEnumClass::Value0));
	EXPECT_TRUE(!!(value & MyEnumClass::Value0));
	EXPECT_TRUE(!!(value | MyEnumClass::Value1));
	EXPECT_FALSE(!!(value & MyEnumClass::Value1));

	EXPECT_NO_THROW(value |= MyEnumClass::Value1);
	EXPECT_TRUE(!!(value | MyEnumClass::Value0));
	EXPECT_TRUE(!!(value & MyEnumClass::Value0));
	EXPECT_TRUE(!!(value | MyEnumClass::Value1));
	EXPECT_TRUE(!!(value & MyEnumClass::Value1));

	EXPECT_NO_THROW(value &= ~MyEnumClass::Value0);
	EXPECT_TRUE(!!(value | MyEnumClass::Value0));
	EXPECT_FALSE(!!(value & MyEnumClass::Value0));
	EXPECT_TRUE(!!(value | MyEnumClass::Value1));
	EXPECT_TRUE(!!(value & MyEnumClass::Value1));

	EXPECT_NO_THROW(value &= ~MyEnumClass::Value1);
	EXPECT_TRUE(!!(value | MyEnumClass::Value0));
	EXPECT_FALSE(!!(value & MyEnumClass::Value0));
	EXPECT_TRUE(!!(value | MyEnumClass::Value1));
	EXPECT_FALSE(!!(value & MyEnumClass::Value1));
}

} // namespace
