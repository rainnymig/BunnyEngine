#pragma once

#include <atomic>

namespace Bunny::Utils
{
enum class TaskState
{
    Pending,
    Running,
    Completed
};

class ITask
{
  public:
    virtual void Run() = 0;
    virtual TaskState GetState() const = 0;
    virtual void Stop() = 0;
};

class BaseTask : public ITask
{
  public:
    virtual void Run() override;
    virtual TaskState GetState() const override;
    virtual void Stop() override;

  protected:
    std::atomic<TaskState> mState{TaskState::Pending};
};

} // namespace Bunny::Utils