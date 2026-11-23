#include "fnd/QIODeviceStreamWrapper.h"

#include <QIODevice>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace HomeCompa;

namespace {

struct MockIODevice : QIODevice
{
	MOCK_METHOD(qint64, readData, (char* data, qint64 maxlen), (override));
	MOCK_METHOD(qint64, writeData, (const char* data, qint64 len), (override));
};

TEST(QIODeviceStreamWrapper, InputNotOpen)
{
	const auto ioStream = std::make_unique<testing::StrictMock<MockIODevice>>();
	auto       stream    = QStdIStream::create(*ioStream);
	ASSERT_TRUE(!!stream);
	int x;
	*stream >> x;
}

TEST(QIODeviceStreamWrapper, InputString)
{
	const auto ioStream = std::make_unique<testing::StrictMock<MockIODevice>>();
	ioStream->open(QIODevice::ReadOnly);
	auto stream = QStdIStream::create(*ioStream);
	ASSERT_TRUE(!!stream);
	std::string str;
	EXPECT_CALL(*ioStream, readData)
		.WillOnce([](char* data, const qint64 maxlen) {
			std::ranges::copy("test", data);
			EXPECT_GE(maxlen, std::size("test"));
			return std::size("test");
		})
		.WillOnce([](char*, qint64) {
			return 0;
		});
	*stream >> str;
	EXPECT_STREQ(str.data(), "test");
}

/*
TEST(QIODeviceStreamWrapper, InputInt)
{
	const auto ioStream = std::make_unique<testing::StrictMock<MockIODevice>>();
	ioStream->open(QIODevice::ReadOnly);
	auto stream = QStdIStream::create(*ioStream);
	ASSERT_TRUE(!!stream);
	int x = 13;
	EXPECT_CALL(*ioStream, readData)
		.WillOnce([](char* data, const qint64 maxlen) {
			*reinterpret_cast<int*>(data) = 42;
			EXPECT_GE(maxlen, sizeof(int));
			return sizeof(int);
		})
		.WillOnce([](char*, qint64) {
			return 0;
		});
	*stream >> x;
	EXPECT_EQ(x, 42);
}
*/
TEST(QIODeviceStreamWrapper, OutputNotOpen)
{
	const auto ioStream = std::make_unique<testing::StrictMock<MockIODevice>>();
	auto       stream    = QStdOStream::create(*ioStream);
	ASSERT_TRUE(!!stream);
	int x = 42;
	*stream << x;
}

TEST(QIODeviceStreamWrapper, OutputString)
{
	const auto ioStream = std::make_unique<testing::StrictMock<MockIODevice>>();
	ioStream->open(QIODevice::WriteOnly);
	auto stream = QStdOStream::create(*ioStream);
	ASSERT_TRUE(!!stream);
	std::string str { "test" };
	bool        called = false;
	EXPECT_CALL(*ioStream, writeData).WillOnce([&](const char* data, qint64 len) {
		EXPECT_STREQ(data, "test");
		called = true;
		return len;
	});
	*stream << str;
	EXPECT_TRUE(called);
}

} // namespace
