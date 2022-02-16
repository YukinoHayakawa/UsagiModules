#include "TaskExecutorSynchronized.hpp"

#include <Usagi/Modules/Common/Logging/Logging.hpp>
#include <Usagi/Runtime/Task/Task.hpp>

namespace usagi
{
std::uint64_t TaskExecutorSynchronized::submit(
    std::unique_ptr<Task> task,
    std::optional<std::vector<std::uint64_t>> wait_on)
{
    LOG(trace, "[Executor] Executing task {}", (void*)task.get());
    run_task(*task);
    return {};
}
}
