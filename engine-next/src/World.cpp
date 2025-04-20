#include "World.h"

#include "WorldComponents.h"

#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace Bunny::Engine
{

void World::update(float deltaTime)
{
    const glm::mat4 rotMat = glm::eulerAngleY<float>(glm::pi<float>() * deltaTime / 4);

    auto transComps = mEntityRegistry.view<TransformComponent>();

    transComps.each([deltaTime, &rotMat](TransformComponent& transComp) {
        transComp.mTransform.mMatrix = rotMat * transComp.mTransform.mMatrix;
    });
}

} // namespace Bunny::Engine
