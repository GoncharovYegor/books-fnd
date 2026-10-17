#include "progress.h"

#include "fnd/ScopedCall.h"

#include "log.h"

using namespace HomeCompa::Util;

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

Progress::Progress(const size_t total, std::string name)
	: m_startedAt { std::chrono::system_clock::now() }
	, m_total { total }
	, m_name { std::move(name) }
{
	PLOGI << std::format("{} started{}", m_name, m_total ? std::format(", total: {}", m_total) : std::string {});
}

Progress::~Progress()
{
	PLOGI << m_name << " completed in " << DurationToString(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - m_startedAt).count());
}

void Progress::Increment(const size_t value, const std::string& comment)
{
	const size_t     count      = (m_count += value);
	const auto       currentPct = static_cast<int>(count * 100 / m_total);
	const ScopedCall guard([&] {
		m_currentPct = currentPct;
	});

	const auto now  = std::chrono::system_clock::now();
	const auto left = count >= m_total ? std::string { "done" } : DurationToString(std::chrono::duration_cast<std::chrono::milliseconds>(m_startedAt + (now - m_startedAt) / count * m_total - now).count());

	if (comment.empty())
	{
		if (m_currentPct != currentPct)
		{
			PLOGV << std::format("{}, {} ({}) {}% - {}", m_name, count, m_total, currentPct, left);
		}
		return;
	}

	PLOGV << std::format("{}, {} ({}) {}% - {}", comment, count, m_total, currentPct, left);
}

size_t Progress::GetCount() const noexcept
{
	return m_count;
}
