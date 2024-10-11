#include "TaskManager.h"

#include "Task.h"

#include <cassert>
#include <algorithm>

namespace Bunny::Utils
{
TaskRunner::TaskRunner() : mIsRunning(true), mPendingTaskCount(0)
{
}

bool TaskRunner::TryAddTask(std::shared_ptr<ITask> task)
{
    std::unique_lock lock(mMutex);

    if (!lock.try_lock())
    {
        return false;
    }

    mTaskQueue.emplace(task);
    mPendingTaskCount = mTaskQueue.size();

    lock.unlock();
    mConditionVar.notify_one();

    return true;
}

void TaskRunner::AddTask(std::shared_ptr<ITask> task)
{
    std::unique_lock lock(mMutex);
    lock.lock();

    mTaskQueue.emplace(task);
    mPendingTaskCount = mTaskQueue.size();

    lock.unlock();
    mConditionVar.notify_one();
}

void TaskRunner::Run()
{
    while (mIsRunning)
    {
        if (mActiveTask != nullptr)
        {
            if (mActiveTask->GetState() == TaskState::Pending)
            {
                mActiveTask->Run();
            }

            if (mActiveTask->GetState() == TaskState::Completed)
            {
                mActiveTask.reset();
            }
        }

        if (!mTaskQueue.empty())
        {
            std::lock_guard lock(mMutex);
            mActiveTask = mTaskQueue.front();
            mTaskQueue.pop();
            mPendingTaskCount = mTaskQueue.size();
        }

        if (mActiveTask == nullptr && mTaskQueue.empty())
        {
            std::unique_lock lock(mMutex);
            mConditionVar.wait(lock, [this]() { return !mTaskQueue.empty(); });
            lock.unlock();
        }
    }

    //  shutdown
    if (mActiveTask != nullptr)
    {
        mActiveTask->Stop();
    }
}

void TaskRunner::Shutdown()
{
    mIsRunning = false;
}

size_t TaskRunner::GetPendingTaskCount() const
{
    return mPendingTaskCount;
}

void TaskDispatcher::StartRunners()
{
    //  only start if it's not started yet
    assert(mRunnerThreads.empty());

    if (mRunners.empty())
    {
        return;
    }

    mRunnerThreads.reserve(mRunners.size());
    for (const auto& runner : mRunners)
    {
        mRunnerThreads.emplace_back(&TaskRunner::Run, runner.get());
    }
}

bool TaskDispatcher::ScheduleTask(std::shared_ptr<ITask> task)
{
    if (mRunners.empty())
    {
        return false;
    }

    auto runnerIt = std::min_element(mRunners.begin(), mRunners.end(),
        [](const std::unique_ptr<TaskRunner>& lhs, const std::unique_ptr<TaskRunner>& rhs) {
            return lhs->GetPendingTaskCount() < rhs->GetPendingTaskCount();
        });

    runnerIt->get()->AddTask(task);

    return true;
}

void TaskDispatcher::Shutdown()
{
    for (const auto& runner : mRunners)
    {
        runner->Shutdown();
    }

    for (std::thread& runnerThread : mRunnerThreads)
    {
        runnerThread.join();
    }

    mRunners.clear();
    mRunnerThreads.clear();
}

void TaskDispatcher::CreateRunners()
{
    const unsigned int threadCount = std::thread::hardware_concurrency();
    mRunners.reserve(threadCount);

    for (int i = 0; i < threadCount; i++)
    {
        // mRunnerThreads.emplace_back(&TaskRunner::Run, runner.get());
        mRunners.emplace_back(std::make_unique<TaskRunner>());
    }
}

} // namespace Bunny::Utils