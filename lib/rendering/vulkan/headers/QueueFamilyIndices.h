#pragma once

#include <optional>

namespace Bunny::Render
{
    struct QueueFamilyIndices {

        using QueueFamilyIndexType = uint32_t;

        std::optional<QueueFamilyIndexType> graphicsFamily;
        std::optional<QueueFamilyIndexType> presentFamily;

        bool isComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };
}