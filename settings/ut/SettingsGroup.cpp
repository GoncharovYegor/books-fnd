#include <atomic>
#include <latch>
#include <thread>

#include <QTemporaryDir>

#include <gtest/gtest.h>

#include "settings/Settings.h"

using namespace HomeCompa;

namespace {

// Each thread reads the key of its own group. If the settings lock is released between selecting the group and reading
// it, another thread's group nests in it ("group0/group1/key") or ends it, and the key reads as missing or another value
TEST(SettingsGroup, ConcurrentGroupsDoNotNest)
{
	const QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const auto settings = SettingsFactory::Create(dir.filePath("settings.ini"));

	constexpr auto threadCount = 4;
	constexpr auto iterations  = 20000;
	for (int i = 0; i < threadCount; ++i)
		settings->Set(QString("group%1/key").arg(i), i, false);

	std::latch                start { threadCount };
	std::atomic_int           failures { 0 };
	std::vector<std::jthread> threads;
	for (int i = 0; i < threadCount; ++i)
		threads.emplace_back([&, i] {
			const auto group = QString("group%1").arg(i);
			start.arrive_and_wait();
			for (int n = 0; n < iterations; ++n)
			{
				const SettingsGroup settingsGroup(*settings, group);
				if (settings->Get("key", -1) != i)
					++failures;
			}
		});
	threads.clear();

	EXPECT_EQ(failures, 0);
}

} // namespace
