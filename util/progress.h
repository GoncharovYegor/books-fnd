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

	struct State
	{
		int64_t timePoint;
		int64_t count;
	};

public:
	Progress(std::string name);
	Progress(int64_t total, std::string name);
	~Progress();
	void    Increment(int64_t value, const std::string& comment = {});
	int64_t GetCount() const noexcept;

private:
	const int64_t     m_startedAt;
	const int64_t     m_total;
	const std::string m_name;

	std::atomic<int>     m_currentPct { 0 };
	std::atomic<int64_t> m_count { 0 };
	std::atomic<State>   m_state;
};

} // namespace HomeCompa::Util
