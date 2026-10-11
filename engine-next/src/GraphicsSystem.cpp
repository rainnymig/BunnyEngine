#include "GraphicsSystem.h"

#include "BunnyError.h"
#include "Config.h"
#include "Engine.h"
#include "TextureBank.h"
#include "MaterialBank.h"
#include "WorldDataTranslator.h"
#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"

namespace Bunny
{
BunnyResult GraphicsSystem::initialize(Engine& engine)
{
    mEngine = &engine;

    mRenderResources = std::make_unique<VulkanRenderResources>();
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(mRenderResources->initialize(&mEngine->getWindow()));

    mRenderer = std::make_unique<VulkanGraphicsRenderer>(mRenderResources.get());
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(mRenderer->initialize(Config::get().mMultiSampleCount));

    mMeshBank = std::make_unique<MeshBank<NormalVertex>>(mRenderResources.get());
    mTextureBank = std::make_unique<TextureBank>(mRenderResources.get(), mRenderer.get());
    mPbrMaterialBank = std::make_unique<PbrMaterialBank>(mRenderResources.get(), mRenderer.get(), mTextureBank.get());

    mWorldTranslator =
        std::make_unique<WorldRenderDataTranslator>(mRenderResources.get(), mRenderer.get(), mMeshBank.get());
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(mWorldTranslator->initialize());

    return BUNNY_HAPPY;
}

BunnyResult GraphicsSystem::draw()
{
    return BunnyResult();
}

BunnyResult GraphicsSystem::shutdown()
{
    return BunnyResult();
}

} // namespace Bunny
