#pragma once

#include <type_traits>

namespace Bunny::Base
{

//  A Bunny Guard won't let you in unless you give them a snack
//  Hint: they like carrots
//  But if they don't like you at all, even carrots won't help you!

template <typename T>
struct BunnySnack
{
    friend T;

  private:
    BunnySnack(const T& t) {}
};

template <typename... Ts>
struct BunnyGuard
{
    template <typename T>
    BunnyGuard(const BunnySnack<T>& c)
    {
        static_assert((std::is_same_v<T, Ts> || ...), "The bunny guard does not like this snack from you :(");
    }
};

#define CARROT Bunny::Base::BunnySnack(*this)

} // namespace Bunny::Base
