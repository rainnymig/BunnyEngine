#include <fmt/core.h>
#include <glm/vec3.hpp>
#include <glm/common.hpp>
#include <glm/gtx/transform.hpp>

#include <iostream>

int main()
{

    glm::vec3 r(1, 0, 0);
    glm::vec3 f(0, 0, -1);

    glm::vec3 u = glm::cross(r, f);
    u = glm::normalize(u);
    fmt::print("{} {} {}", u.x, u.y, u.z);

    return 0;
}