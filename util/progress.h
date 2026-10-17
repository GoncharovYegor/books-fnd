#pragma once

#include <atomic>
#include <chrono>
#include <string>

#include "fnd/NonCopyMovable.h"

#include "export/util.h"

namespace HomeCompa::Util {

class UTIL_EXPORT Progress
{
	NON_COPY_MOVABLE(Progress)

public:
	Progress(std::string name);
	Progress(size_t total, std::string name);
	~Progress();
	void   Increment(size_t value, const std::string& comment = {});
	size_t GetCount() const noexcept;

private:
	const std::chrono::time_point<std::chrono::system_clock> m_startedAt;

	const size_t      m_total;
	const std::string m_name;

	std::atomic<int>    m_currentPct { 0 };
	std::atomic<size_t> m_count { 0 };
};

} // namespace HomeCompa::Util
