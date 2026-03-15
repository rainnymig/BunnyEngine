#include "ImguiHelper.h"

#ifdef _DEBUG
#include <cassert>
#endif

namespace Bunny::Base
{

std::unique_ptr<ImguiHelper> ImguiHelper::msInstance = nullptr;

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
