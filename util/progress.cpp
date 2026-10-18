#include "progress.h"

#include "fnd/ScopedCall.h"
#include "fnd/linear.h"

#include "log.h"

using namespace HomeCompa::Util;

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

Progress::Progress(std::string name)
	: Progress(0, std::move(name))
{
}

Progress::Progress(const int64_t total, std::string name)
	: m_startedAt {
		Now()
}
	, m_total { total }
	, m_name { std::move(name) }
	, m_state { { m_startedAt, 0 } }
{
	PLOGI << std::format("{} started{}", m_name, m_total ? std::format(", total: {}", m_total) : std::string {});
}

Progress::~Progress()
{
	PLOGI << m_name << " completed in " << DurationToString(Now() - m_startedAt);
}

void Progress::Increment(const int64_t value, const std::string& comment)
{
	const int64_t    count      = (m_count += value);
	const auto       currentPct = static_cast<int>(count * 100 / m_total);
	const ScopedCall guard([&] {
		m_currentPct = currentPct;
	});

	const auto now                   = Now();
	const auto [lastTime, lastCount] = m_state.load();
	const Linear l(0LL, lastTime, count - lastCount, now);
	const auto   left = count >= m_total ? std::string { "done" } : DurationToString(l(m_total) - now);

	if (comment.empty())
	{
		if (m_currentPct != currentPct)
		{
			PLOGV << std::format("{}, {} ({}) {}% - {}", m_name, count, m_total, currentPct, left);
			if (count >= lastCount + 10)
				m_state = { now, count };
		}
		return;
	}

	PLOGV << std::format("{}, {} ({}) {}% - {}", comment, count, m_total, currentPct, left);
}

int64_t Progress::GetCount() const noexcept
{
	return m_count;
}
