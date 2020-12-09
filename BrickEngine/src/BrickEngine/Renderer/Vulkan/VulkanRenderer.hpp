#pragma once

#include "BrickEngine/Core/Base.hpp"
#include "BrickEngine/Core/Window.hpp"

#include "BrickEngine/Renderer/Vulkan/VulkanPlatform.hpp"

namespace BrickEngine {

	class VulkanRenderer
	{
	public:
		VulkanRenderer(Window* window);
		~VulkanRenderer();
	private:
		void CreateInstance(std::vector<const char*>& requiredExtentions);
		void SelectPhysicalDevice(std::vector<const char*>& requiredExtentions);
	private:
		Window* m_Window = nullptr;
	private:
		VkInstance m_Instance = nullptr;
#if defined(BRICKENGINE_DEBUG)
		VkDebugUtilsMessengerEXT m_DebugMessenger = nullptr;
#endif
		VkSurfaceKHR m_Surface = nullptr;

		uint32_t m_GraphicsQueueFamilyIndex = -1;
		uint32_t m_PresentQueueFamilyIndex = -1;
		VkSurfaceCapabilitiesKHR m_SurfaceCapabilities = {};
		VkSurfaceFormatKHR m_SurfaceFormat = {};
		VkPresentModeKHR m_PresentMode = {};
		VkPhysicalDevice m_PhysicalDevice = nullptr;

		VkDevice m_Device = nullptr;
	};

}
