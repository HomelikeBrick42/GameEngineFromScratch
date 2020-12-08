#pragma once

#include "BrickEngine/Core/Base.hpp"
#include "BrickEngine/Core/Window.hpp"

#if defined(BRICKENGINE_PLATFORM_WINDOWS)
	#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

namespace BrickEngine {

	class VulkanRenderer
	{
	public:
		VulkanRenderer(Window* window);
		~VulkanRenderer();
	private:
		Window* m_Window = nullptr;
	private:
		VkInstance m_Instance = nullptr;
#if defined(BRICKENGINE_DEBUG)
		VkDebugUtilsMessengerEXT m_DebugMessenger = nullptr;
#endif
		VkPhysicalDevice m_PhysicalDevice = nullptr;
		VkDevice m_Device = nullptr;
	};

}
