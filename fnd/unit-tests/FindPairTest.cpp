#include "fnd/FindPair.h"

#include <gtest/gtest.h>

namespace {

struct FindPairTest : testing::Test
{
	std::pair<int, std::string> array[3] {
		{ 1,     "foo" },
		{ 2, "barsook" },
		{ 3,     "buz" },
	};
	std::vector<std::pair<int, std::string>> vector { std::begin(array), std::end(array) };
	decltype(vector)::const_iterator         begin { vector.cbegin() };
	decltype(vector)::const_iterator         end { vector.cend() };
};

struct Comparer
{
	constexpr bool operator()(const int lhs, const int rhs) const noexcept
	{
		return lhs == rhs + 1;
	}
};

TEST_F(FindPairTest, FindIteratorByFirst)
{
	EXPECT_EQ(FindPairIteratorByFirst(begin, end, 0), end);
	EXPECT_STREQ(FindPairIteratorByFirst(begin, end, 1)->second.data(), "foo");
	EXPECT_STREQ(FindPairIteratorByFirst(begin, end, 2)->second.data(), "barsook");
	EXPECT_STREQ(FindPairIteratorByFirst(begin, end, 3)->second.data(), "buz");
	EXPECT_EQ(FindPairIteratorByFirst(begin, end, 4), end);

	EXPECT_EQ(FindPairIteratorByFirst(vector, 0), end);
	EXPECT_STREQ(FindPairIteratorByFirst(vector, 1)->second.data(), "foo");
	EXPECT_STREQ(FindPairIteratorByFirst(vector, 2)->second.data(), "barsook");
	EXPECT_STREQ(FindPairIteratorByFirst(vector, 3)->second.data(), "buz");
	EXPECT_EQ(FindPairIteratorByFirst(vector, 4), end);

	EXPECT_EQ(FindPairIteratorByFirst(array, 0), std::cend(array));
	EXPECT_STREQ(FindPairIteratorByFirst(array, 1)->second.data(), "foo");
	EXPECT_STREQ(FindPairIteratorByFirst(array, 2)->second.data(), "barsook");
	EXPECT_STREQ(FindPairIteratorByFirst(array, 3)->second.data(), "buz");
	EXPECT_EQ(FindPairIteratorByFirst(array, 4), std::cend(array));
}

TEST_F(FindPairTest, FindIteratorByFirstWithComparer)
{
	EXPECT_EQ(FindPairIteratorByFirst(begin, end, 1, Comparer {}), end);
	EXPECT_STREQ(FindPairIteratorByFirst(begin, end, 2, Comparer {})->second.data(), "foo");
	EXPECT_STREQ(FindPairIteratorByFirst(begin, end, 3, Comparer {})->second.data(), "barsook");
	EXPECT_STREQ(FindPairIteratorByFirst(begin, end, 4, Comparer {})->second.data(), "buz");
	EXPECT_EQ(FindPairIteratorByFirst(begin, end, 5, Comparer {}), end);

	EXPECT_EQ(FindPairIteratorByFirst(vector, 1, Comparer {}), end);
	EXPECT_STREQ(FindPairIteratorByFirst(vector, 2, Comparer {})->second.data(), "foo");
	EXPECT_STREQ(FindPairIteratorByFirst(vector, 3, Comparer {})->second.data(), "barsook");
	EXPECT_STREQ(FindPairIteratorByFirst(vector, 4, Comparer {})->second.data(), "buz");
	EXPECT_EQ(FindPairIteratorByFirst(vector, 5, Comparer {}), end);

	EXPECT_EQ(FindPairIteratorByFirst(array, 1, Comparer {}), std::cend(array));
	EXPECT_STREQ(FindPairIteratorByFirst(array, 2, Comparer {})->second.data(), "foo");
	EXPECT_STREQ(FindPairIteratorByFirst(array, 3, Comparer {})->second.data(), "barsook");
	EXPECT_STREQ(FindPairIteratorByFirst(array, 4, Comparer {})->second.data(), "buz");
	EXPECT_EQ(FindPairIteratorByFirst(array, 5, Comparer {}), std::cend(array));
}

TEST_F(FindPairTest, FindSecond)
{
	EXPECT_DEBUG_DEATH(FindSecond(begin, end, 0), "");
	EXPECT_STREQ(FindSecond(begin, end, 1).data(), "foo");
	EXPECT_STREQ(FindSecond(begin, end, 2).data(), "barsook");
	EXPECT_STREQ(FindSecond(begin, end, 3).data(), "buz");
	EXPECT_DEBUG_DEATH(FindSecond(begin, end, 4), "");

	EXPECT_DEBUG_DEATH(FindSecond(vector, 0), "");
	EXPECT_STREQ(FindSecond(vector, 1).data(), "foo");
	EXPECT_STREQ(FindSecond(vector, 2).data(), "barsook");
	EXPECT_STREQ(FindSecond(vector, 3).data(), "buz");
	EXPECT_DEBUG_DEATH(FindSecond(vector, 4), "");

	EXPECT_DEBUG_DEATH(FindSecond(array, 0), "");
	EXPECT_STREQ(FindSecond(array, 1).data(), "foo");
	EXPECT_STREQ(FindSecond(array, 2).data(), "barsook");
	EXPECT_STREQ(FindSecond(array, 3).data(), "buz");
	EXPECT_DEBUG_DEATH(FindSecond(array, 4), "");
}

TEST_F(FindPairTest, FindSecondWithComparer)
{
	EXPECT_DEBUG_DEATH(FindSecond(begin, end, 1, Comparer {}), "");
	EXPECT_STREQ(FindSecond(begin, end, 2, Comparer {}).data(), "foo");
	EXPECT_STREQ(FindSecond(begin, end, 3, Comparer {}).data(), "barsook");
	EXPECT_STREQ(FindSecond(begin, end, 4, Comparer {}).data(), "buz");
	EXPECT_DEBUG_DEATH(FindSecond(begin, end, 5, Comparer {}), "");

	EXPECT_DEBUG_DEATH(FindSecond(vector, 1, Comparer {}), "");
	EXPECT_STREQ(FindSecond(vector, 2, Comparer {}).data(), "foo");
	EXPECT_STREQ(FindSecond(vector, 3, Comparer {}).data(), "barsook");
	EXPECT_STREQ(FindSecond(vector, 4, Comparer {}).data(), "buz");
	EXPECT_DEBUG_DEATH(FindSecond(vector, 5, Comparer {}), "");

	EXPECT_DEBUG_DEATH(FindSecond(array, 1, Comparer {}), "");
	EXPECT_STREQ(FindSecond(array, 2, Comparer {}).data(), "foo");
	EXPECT_STREQ(FindSecond(array, 3, Comparer {}).data(), "barsook");
	EXPECT_STREQ(FindSecond(array, 4, Comparer {}).data(), "buz");
	EXPECT_DEBUG_DEATH(FindSecond(array, 5, Comparer {}), "");
}

TEST_F(FindPairTest, FindSecondWithDefault)
{
	EXPECT_STREQ(FindSecond(begin, end, 0, "hoy", Comparer {}).data(), "hoy");
	EXPECT_STREQ(FindSecond(begin, end, 1, "hoy", Comparer {}).data(), "hoy");
	EXPECT_STREQ(FindSecond(begin, end, 2, "hoy", Comparer {}).data(), "foo");
	EXPECT_STREQ(FindSecond(begin, end, 3, "hoy", Comparer {}).data(), "barsook");
	EXPECT_STREQ(FindSecond(begin, end, 4, "hoy", Comparer {}).data(), "buz");

	EXPECT_STREQ(FindSecond(vector, 0, "hoy", Comparer {}).data(), "hoy");
	EXPECT_STREQ(FindSecond(vector, 1, "hoy", Comparer {}).data(), "hoy");
	EXPECT_STREQ(FindSecond(vector, 2, "hoy", Comparer {}).data(), "foo");
	EXPECT_STREQ(FindSecond(vector, 3, "hoy", Comparer {}).data(), "barsook");
	EXPECT_STREQ(FindSecond(vector, 4, "hoy", Comparer {}).data(), "buz");

	EXPECT_STREQ(FindSecond(array, 0, "hoy", Comparer {}).data(), "hoy");
	EXPECT_STREQ(FindSecond(array, 1, "hoy", Comparer {}).data(), "hoy");
	EXPECT_STREQ(FindSecond(array, 2, "hoy", Comparer {}).data(), "foo");
	EXPECT_STREQ(FindSecond(array, 3, "hoy", Comparer {}).data(), "barsook");
	EXPECT_STREQ(FindSecond(array, 4, "hoy", Comparer {}).data(), "buz");
}

TEST_F(FindPairTest, FindSecondWithComparerWithDefault)
{
	EXPECT_STREQ(FindSecond(begin, end, 0, "hoy").data(), "hoy");
	EXPECT_STREQ(FindSecond(begin, end, 1, "hoy").data(), "foo");
	EXPECT_STREQ(FindSecond(begin, end, 2, "hoy").data(), "barsook");
	EXPECT_STREQ(FindSecond(begin, end, 3, "hoy").data(), "buz");
	EXPECT_STREQ(FindSecond(begin, end, 4, "hoy").data(), "hoy");

	EXPECT_STREQ(FindSecond(vector, 0, "hoy").data(), "hoy");
	EXPECT_STREQ(FindSecond(vector, 1, "hoy").data(), "foo");
	EXPECT_STREQ(FindSecond(vector, 2, "hoy").data(), "barsook");
	EXPECT_STREQ(FindSecond(vector, 3, "hoy").data(), "buz");
	EXPECT_STREQ(FindSecond(vector, 4, "hoy").data(), "hoy");

	EXPECT_STREQ(FindSecond(array, 0, "hoy").data(), "hoy");
	EXPECT_STREQ(FindSecond(array, 1, "hoy").data(), "foo");
	EXPECT_STREQ(FindSecond(array, 2, "hoy").data(), "barsook");
	EXPECT_STREQ(FindSecond(array, 3, "hoy").data(), "buz");
	EXPECT_STREQ(FindSecond(array, 4, "hoy").data(), "hoy");
}

TEST_F(FindPairTest, FindIteratorBySecond)
{
	EXPECT_EQ(FindPairIteratorBySecond(begin, end, ""), end);
	EXPECT_EQ(FindPairIteratorBySecond(begin, end, "foo")->first, 1);
	EXPECT_EQ(FindPairIteratorBySecond(begin, end, "barsook")->first, 2);
	EXPECT_EQ(FindPairIteratorBySecond(begin, end, "buz")->first, 3);
	EXPECT_EQ(FindPairIteratorBySecond(begin, end, "hoy"), end);

	EXPECT_EQ(FindPairIteratorBySecond(vector, ""), end);
	EXPECT_EQ(FindPairIteratorBySecond(vector, "foo")->first, 1);
	EXPECT_EQ(FindPairIteratorBySecond(vector, "barsook")->first, 2);
	EXPECT_EQ(FindPairIteratorBySecond(vector, "buz")->first, 3);
	EXPECT_EQ(FindPairIteratorBySecond(vector, "hoy"), end);

	EXPECT_EQ(FindPairIteratorBySecond(array, ""), std::end(array));
	EXPECT_EQ(FindPairIteratorBySecond(array, "foo")->first, 1);
	EXPECT_EQ(FindPairIteratorBySecond(array, "barsook")->first, 2);
	EXPECT_EQ(FindPairIteratorBySecond(array, "buz")->first, 3);
	EXPECT_EQ(FindPairIteratorBySecond(array, "hoy"), std::end(array));
}

TEST_F(FindPairTest, FindIteratorBySecondWithPszComparer)
{
	EXPECT_EQ(FindPairIteratorBySecond(begin, end, "", PszComparer {}), end);
	EXPECT_EQ(FindPairIteratorBySecond(begin, end, "foo", PszComparer {})->first, 1);
	EXPECT_EQ(FindPairIteratorBySecond(begin, end, "barsook", PszComparer {})->first, 2);
	EXPECT_EQ(FindPairIteratorBySecond(begin, end, "buz", PszComparer {})->first, 3);
	EXPECT_EQ(FindPairIteratorBySecond(begin, end, "hoy", PszComparer {}), end);

	EXPECT_EQ(FindPairIteratorBySecond(vector, "", PszComparer {}), end);
	EXPECT_EQ(FindPairIteratorBySecond(vector, "foo", PszComparer {})->first, 1);
	EXPECT_EQ(FindPairIteratorBySecond(vector, "barsook", PszComparer {})->first, 2);
	EXPECT_EQ(FindPairIteratorBySecond(vector, "buz", PszComparer {})->first, 3);
	EXPECT_EQ(FindPairIteratorBySecond(vector, "hoy", PszComparer {}), end);

	EXPECT_EQ(FindPairIteratorBySecond(array, "", PszComparer {}), std::end(array));
	EXPECT_EQ(FindPairIteratorBySecond(array, "foo", PszComparer {})->first, 1);
	EXPECT_EQ(FindPairIteratorBySecond(array, "barsook", PszComparer {})->first, 2);
	EXPECT_EQ(FindPairIteratorBySecond(array, "buz", PszComparer {})->first, 3);
	EXPECT_EQ(FindPairIteratorBySecond(array, "hoy", PszComparer {}), std::end(array));
}

TEST_F(FindPairTest, FindIteratorBySecondWithPszComparerCaseInsensitive)
{
	EXPECT_EQ(FindPairIteratorBySecond(begin, end, "", PszComparerCaseInsensitive {}), end);
	EXPECT_EQ(FindPairIteratorBySecond(begin, end, "foO", PszComparerCaseInsensitive {})->first, 1);
	EXPECT_EQ(FindPairIteratorBySecond(begin, end, "BArsOOk", PszComparerCaseInsensitive {})->first, 2);
	EXPECT_EQ(FindPairIteratorBySecond(begin, end, "BUZ", PszComparerCaseInsensitive {})->first, 3);
	EXPECT_EQ(FindPairIteratorBySecond(begin, end, "hoy", PszComparerCaseInsensitive {}), end);

	EXPECT_EQ(FindPairIteratorBySecond(vector, "", PszComparerCaseInsensitive {}), end);
	EXPECT_EQ(FindPairIteratorBySecond(vector, "foO", PszComparerCaseInsensitive {})->first, 1);
	EXPECT_EQ(FindPairIteratorBySecond(vector, "BArsOOk", PszComparerCaseInsensitive {})->first, 2);
	EXPECT_EQ(FindPairIteratorBySecond(vector, "BUZ", PszComparerCaseInsensitive {})->first, 3);
	EXPECT_EQ(FindPairIteratorBySecond(vector, "hoy", PszComparerCaseInsensitive {}), end);

	EXPECT_EQ(FindPairIteratorBySecond(array, "", PszComparerCaseInsensitive {}), std::end(array));
	EXPECT_EQ(FindPairIteratorBySecond(array, "foO", PszComparerCaseInsensitive {})->first, 1);
	EXPECT_EQ(FindPairIteratorBySecond(array, "BArsOOk", PszComparerCaseInsensitive {})->first, 2);
	EXPECT_EQ(FindPairIteratorBySecond(array, "BUZ", PszComparerCaseInsensitive {})->first, 3);
	EXPECT_EQ(FindPairIteratorBySecond(array, "hoy", PszComparerCaseInsensitive {}), std::end(array));
}

TEST_F(FindPairTest, FindFirst)
{
	EXPECT_DEBUG_DEATH(FindFirst(begin, end, ""), "");
	EXPECT_EQ(FindFirst(begin, end, "foo"), 1);
	EXPECT_EQ(FindFirst(begin, end, "barsook"), 2);
	EXPECT_EQ(FindFirst(begin, end, "buz"), 3);
	EXPECT_DEBUG_DEATH(FindFirst(begin, end, "hoy"), "");

	EXPECT_DEBUG_DEATH(FindFirst(vector, ""), "");
	EXPECT_EQ(FindFirst(vector, "foo"), 1);
	EXPECT_EQ(FindFirst(vector, "barsook"), 2);
	EXPECT_EQ(FindFirst(vector, "buz"), 3);
	EXPECT_DEBUG_DEATH(FindFirst(vector, "hoy"), "");

	EXPECT_DEBUG_DEATH(FindFirst(array, ""), "");
	EXPECT_EQ(FindFirst(array, "foo"), 1);
	EXPECT_EQ(FindFirst(array, "barsook"), 2);
	EXPECT_EQ(FindFirst(array, "buz"), 3);
	EXPECT_DEBUG_DEATH(FindFirst(array, "hoy"), "");
}

TEST_F(FindPairTest, FindFirstWithPszComparer)
{
	EXPECT_DEBUG_DEATH(FindFirst(begin, end, "", PszComparer {}), "");
	EXPECT_EQ(FindFirst(begin, end, "foo", PszComparer {}), 1);
	EXPECT_EQ(FindFirst(begin, end, "barsook", PszComparer {}), 2);
	EXPECT_EQ(FindFirst(begin, end, "buz", PszComparer {}), 3);
	EXPECT_DEBUG_DEATH(FindFirst(begin, end, "hoy", PszComparer {}), "");

	EXPECT_DEBUG_DEATH(FindFirst(vector, "", PszComparer {}), "");
	EXPECT_EQ(FindFirst(vector, "foo", PszComparer {}), 1);
	EXPECT_EQ(FindFirst(vector, "barsook", PszComparer {}), 2);
	EXPECT_EQ(FindFirst(vector, "buz", PszComparer {}), 3);
	EXPECT_DEBUG_DEATH(FindFirst(vector, "hoy", PszComparer {}), "");

	EXPECT_DEBUG_DEATH(FindFirst(array, "", PszComparer {}), "");
	EXPECT_EQ(FindFirst(array, "foo", PszComparer {}), 1);
	EXPECT_EQ(FindFirst(array, "barsook", PszComparer {}), 2);
	EXPECT_EQ(FindFirst(array, "buz", PszComparer {}), 3);
	EXPECT_DEBUG_DEATH(FindFirst(array, "hoy", PszComparer {}), "");
}

TEST_F(FindPairTest, FindFirstWithPszComparerCaseInsensitive)
{
	EXPECT_DEBUG_DEATH(FindFirst(begin, end, "", PszComparerCaseInsensitive {}), "");
	EXPECT_EQ(FindFirst(begin, end, "foo", PszComparerCaseInsensitive {}), 1);
	EXPECT_EQ(FindFirst(begin, end, "barsook", PszComparerCaseInsensitive {}), 2);
	EXPECT_EQ(FindFirst(begin, end, "buz", PszComparerCaseInsensitive {}), 3);
	EXPECT_DEBUG_DEATH(FindFirst(begin, end, "hoy", PszComparerCaseInsensitive {}), "");

	EXPECT_DEBUG_DEATH(FindFirst(vector, "", PszComparerCaseInsensitive {}), "");
	EXPECT_EQ(FindFirst(vector, "foo", PszComparerCaseInsensitive {}), 1);
	EXPECT_EQ(FindFirst(vector, "barsook", PszComparerCaseInsensitive {}), 2);
	EXPECT_EQ(FindFirst(vector, "buz", PszComparerCaseInsensitive {}), 3);
	EXPECT_DEBUG_DEATH(FindFirst(vector, "hoy", PszComparerCaseInsensitive {}), "");

	EXPECT_DEBUG_DEATH(FindFirst(array, "", PszComparerCaseInsensitive {}), "");
	EXPECT_EQ(FindFirst(array, "foo", PszComparerCaseInsensitive {}), 1);
	EXPECT_EQ(FindFirst(array, "barsook", PszComparerCaseInsensitive {}), 2);
	EXPECT_EQ(FindFirst(array, "buz", PszComparerCaseInsensitive {}), 3);
	EXPECT_DEBUG_DEATH(FindFirst(array, "hoy", PszComparerCaseInsensitive {}), "");
}

TEST_F(FindPairTest, FindFirstWithDefault)
{
	EXPECT_EQ(FindFirst(begin, end, "", 42), 42);
	EXPECT_EQ(FindFirst(begin, end, "foo", 42), 1);
	EXPECT_EQ(FindFirst(begin, end, "barsook", 42), 2);
	EXPECT_EQ(FindFirst(begin, end, "buz", 42), 3);
	EXPECT_EQ(FindFirst(begin, end, "hoy", 42), 42);

	EXPECT_EQ(FindFirst(vector, "", 42), 42);
	EXPECT_EQ(FindFirst(vector, "foo", 42), 1);
	EXPECT_EQ(FindFirst(vector, "barsook", 42), 2);
	EXPECT_EQ(FindFirst(vector, "buz", 42), 3);
	EXPECT_EQ(FindFirst(vector, "hoy", 42), 42);

	EXPECT_EQ(FindFirst(array, "", 42), 42);
	EXPECT_EQ(FindFirst(array, "foo", 42), 1);
	EXPECT_EQ(FindFirst(array, "barsook", 42), 2);
	EXPECT_EQ(FindFirst(array, "buz", 42), 3);
	EXPECT_EQ(FindFirst(array, "hoy", 42), 42);
}

TEST_F(FindPairTest, FindFirstWithDefaultWithPszComparer)
{
	EXPECT_EQ(FindFirst(begin, end, "", 42, PszComparer {}), 42);
	EXPECT_EQ(FindFirst(begin, end, "foo", 42, PszComparer {}), 1);
	EXPECT_EQ(FindFirst(begin, end, "barsook", 42, PszComparer {}), 2);
	EXPECT_EQ(FindFirst(begin, end, "buz", 42, PszComparer {}), 3);
	EXPECT_EQ(FindFirst(begin, end, "hoy", 42, PszComparer {}), 42);

	EXPECT_EQ(FindFirst(vector, "", 42, PszComparer {}), 42);
	EXPECT_EQ(FindFirst(vector, "foo", 42, PszComparer {}), 1);
	EXPECT_EQ(FindFirst(vector, "barsook", 42, PszComparer {}), 2);
	EXPECT_EQ(FindFirst(vector, "buz", 42, PszComparer {}), 3);
	EXPECT_EQ(FindFirst(vector, "hoy", 42, PszComparer {}), 42);

	EXPECT_EQ(FindFirst(array, "", 42, PszComparer {}), 42);
	EXPECT_EQ(FindFirst(array, "foo", 42, PszComparer {}), 1);
	EXPECT_EQ(FindFirst(array, "barsook", 42, PszComparer {}), 2);
	EXPECT_EQ(FindFirst(array, "buz", 42, PszComparer {}), 3);
	EXPECT_EQ(FindFirst(array, "hoy", 42, PszComparer {}), 42);
}

TEST_F(FindPairTest, FindFirstWithDefaultWithPszComparerCaseInsensitive)
{
	EXPECT_EQ(FindFirst(begin, end, "", 42, PszComparerCaseInsensitive {}), 42);
	EXPECT_EQ(FindFirst(begin, end, "fOo", 42, PszComparerCaseInsensitive {}), 1);
	EXPECT_EQ(FindFirst(begin, end, "baRsoOK", 42, PszComparerCaseInsensitive {}), 2);
	EXPECT_EQ(FindFirst(begin, end, "Buz", 42, PszComparerCaseInsensitive {}), 3);
	EXPECT_EQ(FindFirst(begin, end, "hoy", 42, PszComparerCaseInsensitive {}), 42);

	EXPECT_EQ(FindFirst(vector, "", 42, PszComparerCaseInsensitive {}), 42);
	EXPECT_EQ(FindFirst(vector, "fOo", 42, PszComparerCaseInsensitive {}), 1);
	EXPECT_EQ(FindFirst(vector, "baRsoOK", 42, PszComparerCaseInsensitive {}), 2);
	EXPECT_EQ(FindFirst(vector, "Buz", 42, PszComparerCaseInsensitive {}), 3);
	EXPECT_EQ(FindFirst(vector, "hoy", 42, PszComparerCaseInsensitive {}), 42);

	EXPECT_EQ(FindFirst(array, "", 42, PszComparerCaseInsensitive {}), 42);
	EXPECT_EQ(FindFirst(array, "fOo", 42, PszComparerCaseInsensitive {}), 1);
	EXPECT_EQ(FindFirst(array, "baRsoOK", 42, PszComparerCaseInsensitive {}), 2);
	EXPECT_EQ(FindFirst(array, "Buz", 42, PszComparerCaseInsensitive {}), 3);
	EXPECT_EQ(FindFirst(array, "hoy", 42, PszComparerCaseInsensitive {}), 42);
}

} // namespace
