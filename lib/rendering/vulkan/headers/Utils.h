#pragma once

#include <stack>
#include <functional>

namespace Bunny::Utils
{

template <typename... Args>
class FunctionStack
{
  public:
    //  add a function to the queue
    void AddFunction(std::function<void(Args...)>&& func) { mFuncStack.push(std::move(func)); }

    //  run all functions in reverse order of adding and flush the queue
    void Flush(Args... args)
    {
        while (!mFuncStack.empty())
        {
            mFuncStack.top()(args...);
            mFuncStack.pop();
        }
    }

  private:
    std::stack<std::function<void(Args...)>> mFuncStack;
};
} // namespace Bunny::Utils