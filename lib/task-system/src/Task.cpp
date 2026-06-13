#include "Task.h"

namespace Bunny
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
} // namespace Bunny