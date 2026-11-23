#include "fnd/algorithm.h"

#include <gtest/gtest.h>

using namespace HomeCompa::Util;

namespace {

namespace InBoundsTest {

TEST(InBoundsTest, InBoundsInt)
{
	EXPECT_TRUE(InBounds(1, 0, 2));
	EXPECT_TRUE(InBounds(1, 1, 2));
	EXPECT_FALSE(InBounds(1, 0, 1));
	EXPECT_FALSE(InBounds(1, 2, 1));
	EXPECT_FALSE(InBounds(1, 2, 3));
}

TEST(InBoundsTest, InBoundsDouble)
{
	EXPECT_TRUE(InBounds(1., 0., 2.));
	EXPECT_TRUE(InBounds(1., 1., 2.));
	EXPECT_FALSE(InBounds(1., 0., 1.));
	EXPECT_FALSE(InBounds(1., 2., 1.));
	EXPECT_FALSE(InBounds(1., 2., 3.));
}

}

namespace SetTest {

TEST(SetTest, Set1)
{
	int value = 1;
	EXPECT_FALSE(Set(value, 1));
	EXPECT_TRUE(Set(value, 2));
	EXPECT_EQ(value, 2);
}

TEST(SetTest, Set2)
{
	int        value  = 1;
	bool       called = false;
	const auto call   = [&] {
		called = true;
	};
	EXPECT_FALSE(Set(value, 1, call));
	EXPECT_FALSE(called);
	EXPECT_TRUE(Set(value, 2, call));
	EXPECT_TRUE(called);
	EXPECT_EQ(value, 2);
}

TEST(SetTest, Set3)
{
	int        value        = 1;
	bool       calledBefore = false, calledAfter = false;
	const auto callBefore = [&] {
		calledBefore = true;
	};
	const auto callAfter = [&] {
		calledAfter = true;
	};
	EXPECT_FALSE(Set(value, 1, callBefore, callAfter));
	EXPECT_FALSE(calledBefore);
	EXPECT_FALSE(calledAfter);
	EXPECT_TRUE(Set(value, 2, callBefore, callAfter));
	EXPECT_TRUE(calledBefore);
	EXPECT_TRUE(calledAfter);
	EXPECT_EQ(value, 2);
}

} // namespace SetTest

namespace CreateRangesTest {

struct TestData
{
	std::vector<int>                 items;
	std::vector<std::pair<int, int>> ranges;
};

struct CreateRangesTest : testing::TestWithParam<TestData>
{
};

INSTANTIATE_TEST_SUITE_P(
	CreateRangesTests,
	CreateRangesTest,
	testing::Values(
		TestData {
},
		TestData { { 1 }, { { 1, 2 } } },
		TestData { { 1, 2 }, { { 1, 3 } } },
		TestData { { 10, 11, 12 }, { { 10, 13 } } },
		TestData { { 1, 2, 10, 11, 12 }, { { 1, 3 }, { 10, 13 } } },
		TestData { { 1, 2, 10, 11, 12, 100 }, { { 1, 3 }, { 10, 13 }, { 100, 101 } } }
	)
);

TEST_P(CreateRangesTest, CreateRanges)
{
	const auto& [items, ranges] = GetParam();
	const auto result           = CreateRanges(items);
	EXPECT_TRUE(std::ranges::equal(ranges, result));
}

TEST(CreateRangesFail, Fail)
{
	std::vector<size_t> items { 1, 2, 5, 4 };
	EXPECT_DEBUG_DEATH(CreateRanges(items), "");
}

} // namespace CreateRangesTest

namespace IntersectTest {

TEST(IntersectTest, True)
{
	const std::vector<size_t> item1 { 1, 2, 3, 5 }, item2 { 0, 3, 6 };
	EXPECT_TRUE(Intersect(item1, item2));
}

TEST(IntersectTest, False)
{
	const std::vector<size_t> item1 { 1, 2, 3, 5 }, item2 { 0, 4, 6 };
	EXPECT_FALSE(Intersect(item1, item2));
}

TEST(IntersectTest, Fail)
{
	const std::vector<size_t> item1 { 1, 2, 3, 5 }, item2 { 0, 4, 6, 2 };
	EXPECT_DEBUG_DEATH(Intersect(item1, item2), "");
}

}

namespace FixSizeTest {

struct FixSizeTest : testing::TestWithParam<std::tuple<int, int, int, int, int>>
{
};

INSTANTIATE_TEST_SUITE_P(
	FixSizeTests,
	FixSizeTest,
	testing::Values(
		std::make_tuple(100, 100, 100, 100, 100),
		std::make_tuple(100, 100, 500, 100, 100),
		std::make_tuple(100, 100, 50, 50, 50),
		std::make_tuple(100, 200, 500, 100, 200),
		std::make_tuple(100, 200, 50, 25, 50),
		std::make_tuple(200, 100, 50, 50, 25)
	)
);

TEST_P(FixSizeTest, Test)
{
	const auto& [width, height, threshold, rw, rh] = GetParam();
	int w = width, h = height;
	FixSize(w, h, threshold);
	EXPECT_EQ(w, rw);
	EXPECT_EQ(h, rh);
}

}

namespace CaseInsensitiveComparerTest {

template <typename Left, typename Right = Left>
struct CaseInsensitiveComparerTest : testing::TestWithParam<std::tuple<Left, Right, bool>>
{
	void runCheck()
	{
		const auto& [left, right, expected] = this->GetParam();
		EXPECT_EQ(CaseInsensitiveComparer {}(left, right), expected);
	}
};

#define REGISTER_COMPARER_TEST(SuiteName, LeftType, RightType, ...)              \
    using SuiteName##Fixture = CaseInsensitiveComparerTest<LeftType, RightType>; \
    TEST_P(SuiteName##Fixture, Compare) { this->runCheck(); }                    \
    INSTANTIATE_TEST_SUITE_P(SuiteName, SuiteName##Fixture, testing::Values(__VA_ARGS__))

#define REGISTER_COMPARER_TEST_ITEMS_X_MACRO                                             \
REGISTER_COMPARER_TEST_ITEM(StringAndString, std::string, std::string)                   \
REGISTER_COMPARER_TEST_ITEM(StringAndStringView, std::string, std::string_view)          \
REGISTER_COMPARER_TEST_ITEM(StringViewAndStringView, std::string_view, std::string_view) \
REGISTER_COMPARER_TEST_ITEM(QStringAndString, QString, std::string)

#define REGISTER_COMPARER_TEST_ITEM(NAME, LEFT, RIGHT) \
REGISTER_COMPARER_TEST(                                \
	NAME,                                              \
	LEFT,                                              \
	RIGHT,                                             \
	std::make_tuple("", "", false),                    \
	std::make_tuple("abc", "abc", false),              \
	std::make_tuple("abc", "ABC", false),              \
	std::make_tuple("ABC", "abc", false),              \
	std::make_tuple("ABC", "ABC", false),              \
	std::make_tuple("abc", "abd", true),               \
	std::make_tuple("abc", "ABD", true),               \
	std::make_tuple("ABC", "abd", true),               \
	std::make_tuple("ABC", "ABD", true)                \
);
REGISTER_COMPARER_TEST_ITEMS_X_MACRO
#undef REGISTER_COMPARER_TEST_ITEM

}

namespace CaseInsensitiveHashTest {

struct CaseInsensitiveHashTestF : testing::Test
{
	const std::unordered_set<QString, CaseInsensitiveHash<QString>, CaseInsensitiveEqual<QString>> set { "abc", "abd" };
};

struct CaseInsensitiveHashTest
    : CaseInsensitiveHashTestF
    , testing::WithParamInterface<std::pair<QString, bool>>
{
};

INSTANTIATE_TEST_SUITE_P(
	CaseInsensitiveHashTests,
	CaseInsensitiveHashTest,
	testing::Values(std::make_pair("abc", true), std::make_pair("aBc", true), std::make_pair("ABD", true), std::make_pair("abe", false))
);

TEST_P(CaseInsensitiveHashTest, Test)
{
	const auto& [test, result] = GetParam();
	EXPECT_EQ(set.contains(test), result);
}

}

namespace VariantHashTest {

TEST(VariantHashTest, Test)
{
	EXPECT_NE(VariantHash {}(QVariant(1)), VariantHash {}(QVariant(2)));
	EXPECT_NE(VariantHash {}(QVariant(1)), VariantHash {}(QVariant("1")));
	EXPECT_NE(VariantHash {}(QVariant(1)), VariantHash {}(QVariant(QString { "1" })));
}

}

namespace QStringViewHashTest {

TEST(QStringViewHashTest, TestString)
{
	std::unordered_set<QString, QStringViewHash, QStringViewEqual> data { "abc", "abd" };
	EXPECT_TRUE(data.contains("abc"));
	EXPECT_TRUE(data.contains(QString { "abc" }));
	EXPECT_TRUE(data.contains(QStringView { u"abc" }));
	EXPECT_FALSE(data.contains("abe"));
	EXPECT_FALSE(data.contains(QString { "abe" }));
	EXPECT_FALSE(data.contains(QStringView { u"abe" }));
	data.emplace(QStringView { u"abe" });
	EXPECT_TRUE(data.contains("abe"));
	EXPECT_TRUE(data.contains(QString { "abe" }));
	EXPECT_TRUE(data.contains(QStringView { u"abe" }));
}

TEST(QStringViewHashTest, TestStringView)
{
	std::unordered_set<QStringView, QStringViewHash, QStringViewEqual> data { u"abc", u"abd" };
	EXPECT_TRUE(data.contains("abc"));
	EXPECT_TRUE(data.contains(QString { "abc" }));
	EXPECT_TRUE(data.contains(QStringView { u"abc" }));
	EXPECT_FALSE(data.contains("abe"));
	EXPECT_FALSE(data.contains(QString { "abe" }));
	EXPECT_FALSE(data.contains(QStringView { u"abe" }));
	data.emplace(QStringView { u"abe" });
	EXPECT_TRUE(data.contains("abe"));
	EXPECT_TRUE(data.contains(QString { "abe" }));
	EXPECT_TRUE(data.contains(QStringView { u"abe" }));
}

} // namespace QStringViewHashTest

} // namespace
