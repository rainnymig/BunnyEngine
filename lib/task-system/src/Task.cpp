#include "Task.h"

namespace Bunny::Utils
{
void BaseTask::Run()
{
    mState = TaskState::Completed;
}
TaskState BaseTask::GetState() const
{
    return mState;
}
void BaseTask::Stop()
{
}
} // namespace Bunny::Utils