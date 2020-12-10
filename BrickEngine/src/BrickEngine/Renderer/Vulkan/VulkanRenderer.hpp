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
		void CreateDevice(std::vector<const char*>& requiredExtentions);
		void CreateShader(const std::string& path);
		void OnWindowResize();
		void CreateSwapchain();
		void CreateSwapchainImagesAndViews();
		void CreateRenderPass();
		void CreateGraphicsPipeline();
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
		VkSurfaceFormatKHR m_SurfaceFormat = {};
		VkPresentModeKHR m_PresentMode = {};
		VkPhysicalDevice m_PhysicalDevice = nullptr;

		VkDevice m_Device = nullptr;

		VkQueue m_GraphicsQueue = nullptr;
		VkQueue m_PresentQueue = nullptr;

		std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages = {};

		VkExtent2D m_SwapchainExtent = {};
		std::vector<VkImage> m_SwapchainImages = {};
		std::vector<VkImageView> m_SwapchainImageViews = {};
		VkSwapchainKHR m_Swapchain = nullptr;

		VkPipelineLayout m_PipelineLayout = nullptr;
		VkPipeline m_Pipeline = nullptr;
		VkRenderPass m_RenderPass = nullptr;
	};

}
