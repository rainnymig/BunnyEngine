#pragma once

#include "BunnyResult.h"
#include <fmt/core.h>

#define PRINT_AND_ABORT(message)                                                                                       \
    {                                                                                                                  \
        fmt::print("Fatal error: {}, aborting!", message);                                                             \
        abort();                                                                                                       \
    }

#define PRINT_AND_RETURN(message)                                                                                      \
    {                                                                                                                  \
        fmt::print("Error: {}", message);                                                                              \
        return;                                                                                                        \
    }

#define PRINT_AND_RETURN_VALUE(message, val)                                                                           \
    {                                                                                                                  \
        fmt::print("Error: {}", message);                                                                              \
        return val;                                                                                                    \
    }

#define PRINT_WARNING(message)                                                                                         \
    {                                                                                                                  \
        fmt::print("Warning: {}", message);                                                                            \
    }

#define BUNNY_SUCCESS(val) (val == Bunny::BUNNY_HAPPY)

#define BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(exp) if (BunnyResult tempBunnyResult = exp; !BUNNY_SUCCESS(tempBunnyResult)) { return tempBunnyResult; }