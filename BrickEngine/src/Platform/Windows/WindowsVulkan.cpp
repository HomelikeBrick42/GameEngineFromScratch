#include "brickpch.hpp"

#include "BrickEngine/Renderer/Vulkan/VulkanPlatform.hpp"

#if defined(BRICKENGINE_PLATFORM_WINDOWS)

#include "Platform/Windows/WindowsWindow.hpp"

namespace BrickEngine {

	VkSurfaceKHR VulkanPlatform::CreateSurface(VkInstance instance, Window* window)
	{
		WindowsWindow* windowsWindow = dynamic_cast<WindowsWindow*>(window);
		BRICKENGINE_ASSERT(windowsWindow);

		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
		surfaceCreateInfo.hinstance = windowsWindow->m_Instance;
		surfaceCreateInfo.hwnd = windowsWindow->m_HWND;

		VkSurfaceKHR surface = nullptr;
		BRICKENGINE_ASSERT(vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface) == VK_SUCCESS);
		return surface;
	}

}

#endif
