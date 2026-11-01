#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "export/util.h"

namespace HomeCompa::Util {

class UTIL_EXPORT IExecutor
{
	static std::atomic<size_t> s_id;

public:
	class IThreadHandled // NOLINT(cppcoreguidelines-special-member-functions)
	{
	public:
		virtual ~IThreadHandled() = default;
		using Ptr                 = std::unique_ptr<IThreadHandled>;
	};

	class ThreadContext
	{
	public:
		~ThreadContext()
		{
			m_threadHandled.clear();
		}

	public:
		void Add(IThreadHandled::Ptr threadHandled)
		{
			m_threadHandled.emplace_back(std::move(threadHandled));
		}

	private:
		std::vector<IThreadHandled::Ptr> m_threadHandled;
	};

public:
	using TaskResult = std::function<void(size_t)>;

	struct Task
	{
		std::string                               name;
		std::function<TaskResult(ThreadContext&)> task { [](ThreadContext&) {
			return [](size_t) {
			};
		} };
		size_t id { ++s_id };
	};

public:
	IExecutor()                            = default;
	IExecutor(const IExecutor&)            = delete;
	IExecutor(IExecutor&&)                 = default;
	IExecutor& operator=(const IExecutor&) = delete;
	IExecutor& operator=(IExecutor&&)      = default;
	virtual ~IExecutor()                   = default;

	virtual size_t operator()(Task&& task, int priority = 0, bool displace = true) = 0;

	size_t operator()(Task&& task, const int priority = 0, const bool displace = true) const
	{
		return (*const_cast<IExecutor*>(this))(std::move(task), priority, displace);
	}

	virtual void Stop() = 0;
};

} // namespace HomeCompa::Util
