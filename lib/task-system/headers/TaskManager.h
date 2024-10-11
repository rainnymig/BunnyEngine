#pragma once

#include <queue>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>

namespace Bunny::Utils
{

class ITask;

class TaskRunner
{
  public:
    TaskRunner();

    // //  delete because of mutex is not copyable
    // TaskRunner(const TaskRunner& other) = delete;
    // TaskRunner(TaskRunner&& other) noexcept = default;

    bool TryAddTask(std::shared_ptr<ITask> task);
    void AddTask(std::shared_ptr<ITask> task);
    void Run();
    void Shutdown();

    size_t GetPendingTaskCount() const;

  private:
    std::queue<std::shared_ptr<ITask>> mTaskQueue;
    std::shared_ptr<ITask> mActiveTask;
    mutable std::mutex mMutex;
    std::condition_variable mConditionVar;
    std::atomic_bool mIsRunning;
    std::atomic_size_t mPendingTaskCount;
};

class TaskDispatcher
{
  public:
    void StartRunners();
    bool ScheduleTask(std::shared_ptr<ITask> task);
    void Shutdown();

  private:
    void CreateRunners();

    std::vector<std::unique_ptr<TaskRunner>> mRunners;
    std::vector<std::thread> mRunnerThreads;
};

} // namespace Bunny::Utils