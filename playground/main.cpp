#include "Timer.h"

#include <glm/vec4.hpp>
#include <glm/matrix.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <fmt/core.h>

#include <array>
#include <random>

using IdType = uint32_t;

struct ObjectData
{
    glm::mat4 model;
    glm::mat4 invTransModel;
    IdType meshId;
    glm::vec3 padding;
};

int main()
{
    fmt::print("size: {}, align: {}\n", sizeof(ObjectData), alignof(ObjectData));

    return 0;
}