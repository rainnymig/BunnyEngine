#pragma once

#include "BunnyResult.h"

#include <vulkan/vk_enum_string_helper.h>
#include <fmt/core.h>

#define VK_HARD_CHECK(exp)                                                                                             \
    {                                                                                                                  \
        VkResult r = exp;                                                                                              \
        if (r)                                                                                                         \
        {                                                                                                              \
            fmt::print("Vulkan error on hard check: {}, aborting!", string_VkResult(r));                               \
            abort();                                                                                                   \
        }                                                                                                              \
    }

#define VK_SOFT_CHECK(exp)                                                                                             \
    {                                                                                                                  \
        VkResult r = exp;                                                                                              \
        if (r)                                                                                                         \
        {                                                                                                              \
            fmt::print("Vulkan error on soft check: {}.", string_VkResult(r));                                         \
        }                                                                                                              \
    }

#define VK_CHECK_OR_RETURN(exp)                                                                                        \
    if (VkResult r = exp; r != VK_SUCCESS)                                                                             \
    {                                                                                                                  \
        fmt::print("Vulkan error: {}, return.", string_VkResult(r));                                                   \
        return;                                                                                                        \
    }

#define VK_CHECK_OR_RETURN_BUNNY_SAD(exp)                                                                              \
    if (VkResult r = exp; r != VK_SUCCESS)                                                                             \
    {                                                                                                                  \
        fmt::print("Vulkan error: {}, return BUNNY_SAD.", string_VkResult(r));                                         \
        return BUNNY_SAD;                                                                                              \
    }

#define VK_CHECK_OR_RETURN_VALUE(exp, val)                                                                             \
    if (VkResult r = exp; r != VK_SUCCESS)                                                                             \
    {                                                                                                                  \
        fmt::print("Vulkan error: {}, return value.", string_VkResult(r));                                             \
        return val;                                                                                                    \
    }