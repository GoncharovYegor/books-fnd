#pragma once

#include <string>

#include "fnd/NonCopyMovable.h"
#include "fnd/memory.h"

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
	struct Impl;
	PropagateConstPtr<Impl> m_impl;
};

} // namespace HomeCompa::Util
