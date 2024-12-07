#pragma once

#include <vulkan/vk_enum_string_helper.h>
#include <fmt/core.h>

#define VkHardCheck(exp)                                                                                               \
    {                                                                                                                  \
        VkResult r = exp;                                                                                              \
        if (r)                                                                                                         \
        {                                                                                                              \
            fmt::print("Vulkan error on hard check: {}, aborting!", string_VkResult(r));                               \
            abort();                                                                                                   \
        }                                                                                                              \
    }

#define VkSoftCheck(exp)                                                                                               \
    {                                                                                                                  \
        VkResult r = exp;                                                                                              \
        if (r)                                                                                                         \
        {                                                                                                              \
            fmt::print("Vulkan error on soft check: {}.", string_VkResult(r));                                         \
        }                                                                                                              \
    }