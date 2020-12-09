#pragma once

#include "BrickEngine/Core/Base.hpp"
#include "BrickEngine/Core/Window.hpp"

#if defined(BRICKENGINE_PLATFORM_WINDOWS)
	#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

namespace BrickEngine {

	class VulkanPlatform
	{
	public:
		VulkanPlatform() = delete;

		static VkSurfaceKHR CreateSurface(VkInstance instance, Window* window);
	};

}
