#pragma once

#include <memory>
#ifdef _DEBUG
#include <cassert>
#endif

namespace Bunny::Base
{

template <typename T>
class Singleton
{
  public:
    static void setup();
    static T& get();

  protected:
    static std::unique_ptr<T> msInstance;
};

template <typename T>
void Singleton<T>::setup()
{
#ifdef _DEBUG
    assert(msInstance == nullptr);
#endif

    msInstance = std::make_unique<T>();
}

template <typename T>
T& Singleton<T>::get()
{
#ifdef _DEBUG
    assert(msInstance != nullptr);
#endif

    return *msInstance;
}

} // namespace Bunny::Base
