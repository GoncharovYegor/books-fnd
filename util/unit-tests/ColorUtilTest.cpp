#include "util/ColorUtil.h"

#include <QColor>
#include <QString>
#include <QVariant>

#include <gtest/gtest.h>

#include "mock/settings/MockSettings.h"

using namespace HomeCompa::Test;
using namespace HomeCompa::Util;

namespace {

constexpr auto KEY = "key";

struct ColorUtil_GetColor : testing::Test
{
	const testing::StrictMock<MockSettings> settings;
};

TEST_F(ColorUtil_GetColor, NoKey)
{
	EXPECT_CALL(settings, Get(testing::_, testing::Matcher<const QVariant&>(testing::_))).WillOnce(testing::Return(QVariant {}));
	const auto color = GetColor(settings, KEY);
	EXPECT_FALSE(color.has_value());
}

TEST_F(ColorUtil_GetColor, BadValue)
{
	EXPECT_CALL(settings, Get(testing::_, testing::Matcher<const QVariant&>(testing::_))).WillOnce(testing::Return(QVariant { "Bad value" }));
	const auto color = GetColor(settings, KEY);
	EXPECT_FALSE(color.has_value());
}

TEST_F(ColorUtil_GetColor, Transparent)
{
	EXPECT_CALL(settings, Get(testing::_, testing::Matcher<const QVariant&>(testing::_))).WillOnce(testing::Return(QVariant { 0xFF000000 }));
	const auto color = GetColor(settings, KEY);
	EXPECT_TRUE(color.has_value());
	EXPECT_TRUE(color->isValid());
	EXPECT_EQ(*color, QColorConstants::Transparent);
}

struct ColorUtil_GetColorTest
    : ColorUtil_GetColor
    , testing::WithParamInterface<std::tuple<uint32_t, int, int, int, int>>
{
};

INSTANTIATE_TEST_SUITE_P(
	ColorUtil_GetColorTests,
	ColorUtil_GetColorTest,
	testing::Values(std::make_tuple(0x42, 0x42, 0, 0, 0), std::make_tuple(0x4200, 0, 0x42, 0, 0), std::make_tuple(0x420000, 0, 0, 0x42, 0), std::make_tuple(0x04030201, 1, 2, 3, 4))
);

TEST_P(ColorUtil_GetColorTest, Get)
{
	const auto [value, r, g, b, a] = GetParam();
	EXPECT_CALL(settings, Get(testing::_, testing::Matcher<const QVariant&>(testing::_))).WillOnce(testing::Return(QVariant { value }));
	const auto color = GetColor(settings, KEY);
	EXPECT_TRUE(color.has_value());
	QColor test(r, g, b, 255 - a);
	EXPECT_EQ(*color, test);
}

} // namespace
