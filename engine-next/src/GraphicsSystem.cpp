#include "GraphicsSystem.h"

#include "BunnyError.h"
#include "Config.h"
#include "Engine.h"
#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"

namespace Bunny
{
BunnyResult GraphicsSystem::initialize(Engine& engine)
{

    mEngine = &engine;

    mRenderResources = std::make_unique<VulkanRenderResources>();
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(mRenderResources->initialize(&mEngine->getWindow()));

    mRenderer = std::make_unique<VulkanGraphicsRenderer>();
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(mRenderer->initialize(Config::get().mMultiSampleCount));

    return BUNNY_HAPPY;
}
} // namespace Bunny
