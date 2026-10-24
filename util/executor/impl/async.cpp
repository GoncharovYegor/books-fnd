#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

#include <QCoreApplication>
#include <QTimer>

#include "fnd/NonCopyMovable.h"

#include "executor/factory.h"

#include "FunctorExecutionForwarder.h"
#include "IExecutor.h"
#include "log.h"

namespace HomeCompa::Util::ExecutorPrivate::Async {

namespace {

class IPool // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
	virtual ~IPool()                   = default;
	virtual void Work(std::stop_token) = 0;
};

class Thread
{
public:
	explicit Thread(IPool& pool)
		: m_thread(std::bind_front(&IPool::Work, std::ref(pool)))
	{
	}

private:
	std::jthread m_thread;
};

class Executor final
    : public IExecutor
    , public IPool
{
	NON_COPY_MOVABLE(Executor)

public:
	explicit Executor(ExecutorInitializer initializer)
		: m_initializer { std::move(initializer) }
		, m_forwarder { new FunctorExecutionForwarder }
	{
		const auto cpuCount       = static_cast<int>(std::thread::hardware_concurrency());
		const auto maxThreadCount = std::min(std::max(cpuCount - 2, 1), m_initializer.maxThreadCount);
		std::generate_n(std::back_inserter(m_threads), maxThreadCount, [&]() {
			auto thread = std::make_unique<Thread>(*this);
			return thread;
		});

		PLOGD << std::format("{} thread(s) executor created", std::size(m_threads));
	}

	~Executor() override
	{
		Stop();

		// While the application object is being destroyed, Qt discards the still-queued events under the event-queue lock, and
		// one of them may hold the last reference to this executor. Both posting the deferred delete and destroying the forwarder
		// (any QObject) take that same lock and would deadlock, so the forwarder is deliberately left for the exiting process
		if (!QCoreApplication::instance() || QCoreApplication::closingDown())
			return;

		QTimer::singleShot(0, [forwarder = m_forwarder] {
			delete forwarder;
		});
	}

private: // Util::IExecutor
	size_t operator()(Task&& task, int priority, const bool displace) override
	{
		const auto id = task.id;
		{
			std::lock_guard lock(m_tasksGuard);
			if (!priority)
				priority = 1024;
			else if (displace)
				m_tasks.erase(priority);

			m_tasks.emplace(priority, std::move(task));
			m_condition.notify_one();
		}

		return id;
	}

	void Stop() override
	{
		PLOGD << std::format("stop {} thread(s) executor", std::size(m_threads));
		m_threads.clear();
	}

private:
	void Work(const std::stop_token stop) override
	{
		while (!stop.stop_requested())
		{
			auto taskOpt = [&]() -> std::optional<Task> {
				std::unique_lock lock(m_tasksGuard);
				if (!m_condition.wait(lock, stop, [this]() {
						return !m_tasks.empty();
					}))
					return std::nullopt;

				assert(!m_tasks.empty());
				const auto it           = m_tasks.begin();
				auto       returnedTask = std::move(it->second);
				m_tasks.erase(it);
				return returnedTask;
			}();

			if (!taskOpt)
				continue;

			const auto& [name, task, id] = *taskOpt;

			m_forwarder->Forward(m_initializer.beforeExecute);
			try
			{
				PLOGD << name << " started";
				auto taskResult = task();
				PLOGD << name << " finished";

				m_forwarder->Forward([id, taskResult = std::move(taskResult)] {
					taskResult(id);
				});
			}
			catch (const std::exception& ex)
			{
				PLOGE << name << ": " << ex.what();
			}
			catch (...)
			{
				PLOGE << name << " failed";
			}
			m_forwarder->Forward(m_initializer.afterExecute);
		}

		m_initializer.onDestroy();
	}

private:
	const ExecutorInitializer            m_initializer;
	std::condition_variable_any          m_condition;
	std::mutex                           m_tasksGuard;
	std::multimap<int, Task>             m_tasks;
	FunctorExecutionForwarder*           m_forwarder { nullptr };
	std::vector<std::unique_ptr<Thread>> m_threads;
};

} // namespace

std::unique_ptr<IExecutor> CreateExecutor(ExecutorInitializer initializer)
{
	return std::make_unique<Executor>(std::move(initializer));
}

} // namespace HomeCompa::Util::ExecutorPrivate::Async
