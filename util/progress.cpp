#include "progress.h"

#include <atomic>
#include <chrono>

#include "fnd/ScopedCall.h"
#include "fnd/linear.h"

#include "log.h"

using namespace HomeCompa::Util;

namespace{

int64_t Now() noexcept
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string DurationToString(long long milliseconds)
{
	if (milliseconds < 0)
		return "done";

	auto seconds = milliseconds / 1000;
	if (seconds == 0)
		return std::format("{:03}ms", milliseconds);

	milliseconds %= 1000;
	auto minutes  = seconds / 60;
	if (minutes == 0)
		return std::format("{:02}.{:03}s", seconds, milliseconds);

	seconds    %= 60;
	auto hours  = minutes / 60;
	if (hours == 0)
		return std::format("{:02}m {:02}s", minutes, seconds);

	minutes %= 60;
	return std::format("{}h {:02}m {:02}s", hours, minutes, seconds);
}

}

struct Progress::Impl
{
    const std::string name;
    const int64_t     total;
    const int64_t     startedAt {Now() };

    std::atomic<int>     currentPct { 0 };
    std::atomic<int64_t> count { 0 };
    std::atomic<State>   state { { startedAt, 0 } };

};

Progress::Progress(std::string name)
	: Progress(0, std::move(name))
{
}

Progress::Progress(const int64_t total, std::string name)
    : m_impl{std::move(name), total}
{
	PLOGI << std::format("{} started{}", m_impl->name, total ? std::format(", total: {}", total) : std::string {});
}

Progress::~Progress()
{
    auto& impl = *m_impl;
    PLOGI << impl.name << " completed in " << DurationToString(Now() - impl.startedAt);
}

void Progress::Increment(const int64_t value, const std::string& comment)
{
    auto& impl = *m_impl;
    const int64_t    count      = (impl.count += value);
    const auto       currentPct = static_cast<int>(count * 100 / impl.total);
	const ScopedCall guard([&] {
        impl.currentPct = currentPct;
	});

	const auto now                   = Now();
    const auto [lastTime, lastCount] = impl.state.load();
	const Linear<int64_t, int64_t> l(0, lastTime, count - lastCount, now);
    const auto   left = count >= impl.total ? std::string { "done" } : DurationToString(l(impl.total) - now);

	if (comment.empty())
	{
        if (impl.currentPct != currentPct)
		{
            PLOGV << std::format("{}, {} ({}) {}% - {}", impl.name, count, impl.total, currentPct, left);
			if (count >= lastCount + 10)
                impl.state = { now, count };
		}
		return;
	}

    PLOGV << std::format("{}, {} ({}) {}% - {}", comment, count, impl.total, currentPct, left);
}

int64_t Progress::GetCount() const noexcept
{
    return m_impl->count;
}
