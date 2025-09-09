#include "ImguiHelper.h"

#ifdef _DEBUG
#include <cassert>
#endif

namespace Bunny::Base
{

std::unique_ptr<ImguiHelper> ImguiHelper::msInstance = nullptr;

void ImguiHelper::setup()
{
#ifdef _DEBUG
    assert(msInstance == nullptr);
#endif

    msInstance = std::make_unique<ImguiHelper>();
}

void ImguiHelper::registerCommand(ImguiCommand command)
{
    mRegisteredCommands.emplace_back(std::move(command));
}

void ImguiHelper::render() const
{
    for (const ImguiCommand& command : mRegisteredCommands)
    {
        command();
    }
}
} // namespace Bunny::Base
