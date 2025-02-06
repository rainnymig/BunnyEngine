#pragma once

#define PRINT_AND_ABORT(message)                                                                                       \
    {                                                                                                                  \
        fmt::print("Fatal error: {}, aborting!", message);                                                             \
        abort();                                                                                                       \
    }

#define PRINT_AND_RETURN(message)                                                                                      \
    {                                                                                                                  \
        fmt::print("Fatal error: {}, aborting!", message);                                                             \
        return;                                                                                                        \
    }

#define PRINT_AND_RETURN_VALUE(message, val)                                                                           \
    {                                                                                                                  \
        fmt::print("Fatal error: {}, aborting!", message);                                                             \
        return val;                                                                                                    \
    }