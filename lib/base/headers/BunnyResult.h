#pragma once

namespace Bunny
{
enum BunnyResult
{
    BUNNY_HAPPY = 0,
    BUNNY_SAD = 1
};

#define BUNNY_SUCCESS(val) (val == Bunny::BUNNY_HAPPY)

#define BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(exp)                                                                      \
    if (BunnyResult tempBunnyResult = exp; !BUNNY_SUCCESS(tempBunnyResult))                                            \
    {                                                                                                                  \
        return tempBunnyResult;                                                                                        \
    }

} // namespace Bunny
