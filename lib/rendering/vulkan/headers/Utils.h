#pragma once

#include <deque>
#include <functional>

namespace Bunny::Utils
{

template <typename... Args>
class FunctionStack
{
  public:
    //  add a function to the queue
    void AddFunction(std::function<void(Args...)>&& func) { mFunctionQueue.push_back(std::move(func)); }

    //  run all functions in reverse order of adding and flush the queue
    void Flush(Args... args)
    {
        while (!mFunctionQueue.empty())
        {
            mFunctionQueue.back()(args...);
            mFunctionQueue.pop_back();
        }
    }

  private:
    std::deque<std::function<void(Args...)>> mFunctionQueue;
};
} // namespace Bunny::Utils