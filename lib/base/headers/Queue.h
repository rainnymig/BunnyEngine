#pragma once

#include <array>
#include <mutex>
#include <atomic>
#include <functional>
#include <list>

namespace Bunny::Base
{
template <typename ElementT>
class LockFreeSingleConsumerQueue
{
};

template <typename EventT>
class EventQueue
{
  public:
    using EventHandler = std::function<void(const EventT&)>;
    using EventHandlerHandle = std::list<EventHandler>::const_iterator;

    EventHandlerHandle registerEventHandler(EventHandler&& handler);
    void unregisterEventHandler(EventHandlerId id);

    void enqueueEvent(EventT&& event);
    void processAllEvents();

  protected:
    LockFreeSingleConsumerQueue<EventT> mQueue;
    std::list<EventHandler> mHandlers;
};

} // namespace Bunny::Base
