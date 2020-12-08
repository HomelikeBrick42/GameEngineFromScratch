#include "brickpch.hpp"
#include "BrickEngine/Core/Window.hpp"

#if defined(BRICKENGINE_PLATFORM_WINDOWS)
    #include "Platform/Windows/WindowsWindow.hpp"
#endif

namespace BrickEngine {

    Window* Window::Create(uint32_t width, uint32_t height, const std::string& title)
    {
#if defined(BRICKENGINE_PLATFORM_WINDOWS)
        return new WindowsWindow(width, height, title);
#else
        return nullptr;
#endif
    }

}
