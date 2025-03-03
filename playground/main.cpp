#include "Timer.h"

#include <glm/vec4.hpp>
#include <glm/matrix.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <fmt/core.h>
#include <entt/entt.hpp>

#include <array>
#include <random>

using IdType = size_t;

struct MeshComponent
{
    IdType mMaterialId;
    IdType mMaterialInstanceId;
};

struct TransformComponent
{
    glm::mat4x4 mTransformMatrix;
};

size_t getRandom()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, 3);
    return dist(gen);
}

int main()
{
    entt::registry gameObjectRegistry;

    constexpr std::array<IdType, 3> materials{1, 2, 3};
    constexpr std::array<IdType, 9> materialInstances{11, 12, 13, 21, 22, 23, 31, 32, 33};

    constexpr size_t entityCount = 20;

    //  create entities with components
    for (size_t i = 0; i < entityCount; i++)
    {
        const auto entity = gameObjectRegistry.create();
        size_t matIdx = i % materials.size();
        auto& m = gameObjectRegistry.emplace<MeshComponent>(entity);
        m.mMaterialId = materials[matIdx];
        m.mMaterialInstanceId = m.mMaterialId * 10 + getRandom();
        auto& t = gameObjectRegistry.emplace<TransformComponent>(entity);
        t.mTransformMatrix = glm::translate(glm::mat4(1), glm::vec3{i, i, i});
    }

    gameObjectRegistry.sort<MeshComponent>([](const MeshComponent& lhs, const MeshComponent& rhs) {
        return lhs.mMaterialId < rhs.mMaterialId ||
               (lhs.mMaterialId == rhs.mMaterialId && lhs.mMaterialInstanceId < rhs.mMaterialInstanceId);
    });
    gameObjectRegistry.sort<TransformComponent, MeshComponent>();

    //  iterate the components
    auto view = gameObjectRegistry.view<const MeshComponent, const TransformComponent>();
    for (const auto [entity, mesh, transform] : view.each())
    {
        fmt::print("entity: mat id {}, mat inst id {}.\n", mesh.mMaterialId, mesh.mMaterialInstanceId);
    }

    return 0;
}