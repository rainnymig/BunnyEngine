#include <fmt/core.h>
#include <glm/glm.hpp>

void printVec(const glm::vec3& v)
{
    fmt::print("({}, {}, {})\n", v.x, v.y, v.z);
}

int main()
{
    glm::vec3 v(1.0f, 2.0f, 3.0f);
    printVec(v);
    glm::vec3 v2 = v + 3.0f;
    printVec(v2);
    printVec(glm::cross(v, v2));
    glm::vec3 v3 = v + 3.0f * v2;
    printVec(v3);
    return 0;
}