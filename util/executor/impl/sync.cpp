#include <memory>

#include "fnd/NonCopyMovable.h"

#include "executor/factory.h"

#include "IExecutor.h"
#include "log.h"

namespace HomeCompa::Util::ExecutorPrivate::Sync {

namespace {

class Executor final : public IExecutor
{
	NON_COPY_MOVABLE(Executor)

public:
	explicit Executor(ExecutorInitializer initializer)
		: m_initializer(std::move(initializer))
	{
		m_initializer.onCreate();
	}

	~Executor() override
	{
		m_initializer.onDestroy();
	}

private: // Util::IExecutor
	size_t operator()(Task&& task, int /*priority*/, bool) override
	{
		ThreadContext ctx;
		m_initializer.beforeExecute();
		PLOGD << task.name << " started";
		const auto taskResult = task.task(ctx);
		PLOGD << task.name << " finished";
		taskResult(task.id);
		m_initializer.afterExecute();
		return task.id;
	}

	void Stop() override
	{
	}

private:
	const ExecutorInitializer m_initializer;
};

} // namespace

std::unique_ptr<Util::IExecutor> CreateExecutor(ExecutorInitializer initializer)
{
	return std::make_unique<Executor>(std::move(initializer));
}

} // namespace HomeCompa::Util::ExecutorPrivate::Sync
