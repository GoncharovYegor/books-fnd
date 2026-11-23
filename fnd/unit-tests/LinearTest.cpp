#include "fnd/linear.h"

#include <gtest/gtest.h>

using namespace HomeCompa;

namespace {

TEST(Linear, IntAsc)
{
	Linear l(0, 0, 100, 100);
	EXPECT_EQ(l.x(), std::make_pair(0, 100));
	EXPECT_EQ(l.y(), std::make_pair(0, 100));
	EXPECT_EQ(l(-101), -101);
	EXPECT_EQ(l(-1), -1);
	EXPECT_EQ(l(0), 0);
	EXPECT_EQ(l(42), 42);
	EXPECT_EQ(l(50), 50);
	EXPECT_EQ(l(99), 99);
	EXPECT_EQ(l(100), 100);
	EXPECT_EQ(l(101), 101);
	EXPECT_EQ(l(100500), 100500);
}

TEST(Linear, IntDesc)
{
	Linear l(0, 100, 100, 0);
	EXPECT_EQ(l.x(), std::make_pair(0, 100));
	EXPECT_EQ(l.y(), std::make_pair(100, 0));
	EXPECT_EQ(l(-101), 201);
	EXPECT_EQ(l(-1), 101);
	EXPECT_EQ(l(0), 100);
	EXPECT_EQ(l(42), 58);
	EXPECT_EQ(l(50), 50);
	EXPECT_EQ(l(99), 1);
	EXPECT_EQ(l(100), 0);
	EXPECT_EQ(l(101), -1);
	EXPECT_EQ(l(100500), -100400);
}

TEST(Linear, DoubleAsc)
{
	Linear l(0., 0., 100., 100.);
	EXPECT_EQ(l.x(), std::make_pair(0., 100.));
	EXPECT_EQ(l.y(), std::make_pair(0., 100.));
	EXPECT_DOUBLE_EQ(l(-101.), -101.);
	EXPECT_DOUBLE_EQ(l(-1.), -1.);
	EXPECT_DOUBLE_EQ(l(0.), 0.);
	EXPECT_DOUBLE_EQ(l(42.), 42.);
	EXPECT_DOUBLE_EQ(l(50.), 50.);
	EXPECT_DOUBLE_EQ(l(99.), 99.);
	EXPECT_DOUBLE_EQ(l(100.), 100.);
	EXPECT_DOUBLE_EQ(l(101.), 101.);
	EXPECT_DOUBLE_EQ(l(100500.), 100500.);
}

TEST(Linear, DoubleDesc)
{
	Linear l(0., 100., 100., 0.);
	EXPECT_EQ(l.x(), std::make_pair(0., 100.));
	EXPECT_EQ(l.y(), std::make_pair(100., 0.));
	EXPECT_DOUBLE_EQ(l(-101.), 201.);
	EXPECT_DOUBLE_EQ(l(-1.), 101.);
	EXPECT_DOUBLE_EQ(l(0.), 100.);
	EXPECT_DOUBLE_EQ(l(42.), 58.);
	EXPECT_DOUBLE_EQ(l(50.), 50.);
	EXPECT_DOUBLE_EQ(l(99.), 1.);
	EXPECT_DOUBLE_EQ(l(100.), 0.);
	EXPECT_DOUBLE_EQ(l(101.), -1.);
	EXPECT_DOUBLE_EQ(l(100500.), -100400.);
}

} // namespace
