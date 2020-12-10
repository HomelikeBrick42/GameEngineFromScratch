#include "brickpch.hpp"
#include "BrickEngine/Renderer/Vulkan/VulkanRenderer.hpp"

#define VK_CHECK(x) { \
	VkResult result = x; BRICKENGINE_ASSERT(result == VK_SUCCESS) \
}

#undef min
#undef max

namespace BrickEngine {

	static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT          messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT                 messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT*		pCallbackData,
		void*											pUserData)
	{
		switch (messageSeverity)
		{
		default:
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			Log::Trace(pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			Log::Info(pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			Log::Warn(pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			Log::Error(pCallbackData->pMessage);
			break;
		}

		return VK_FALSE;
	}

	VulkanRenderer::VulkanRenderer(Window* window)
		: m_Window(window)
	{
		std::vector<const char*> instanceExtentions = {
#if defined(BRICKENGINE_PLATFORM_WINDOWS)
		   VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#else
   #error Unsupported platform!
#endif
		   VK_KHR_SURFACE_EXTENSION_NAME
		};
		CreateInstance(instanceExtentions);
		BRICKENGINE_ASSERT(m_Instance);

		m_Surface = VulkanPlatform::CreateSurface(m_Instance, m_Window);
		BRICKENGINE_ASSERT(m_Surface);

		std::vector<const char*> deviceExtentions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		SelectPhysicalDevice(deviceExtentions);
		BRICKENGINE_ASSERT(m_PhysicalDevice);

		CreateDevice(deviceExtentions);
		BRICKENGINE_ASSERT(m_Device);

		vkGetDeviceQueue(m_Device, m_GraphicsQueueFamilyIndex, 0, &m_GraphicsQueue);
		BRICKENGINE_ASSERT(m_GraphicsQueue);
		vkGetDeviceQueue(m_Device, m_PresentQueueFamilyIndex, 0, &m_PresentQueue);
		BRICKENGINE_ASSERT(m_PresentQueue);

		CreateShader("assets/shaders/main");
		BRICKENGINE_ASSERT(m_ShaderStages.size() == 2);

		OnWindowResize();

		CreateRenderPass();
		BRICKENGINE_ASSERT(m_RenderPass);

		CreateGraphicsPipeline();
		BRICKENGINE_ASSERT(m_PipelineLayout);
		BRICKENGINE_ASSERT(m_Pipeline);
	}

	VulkanRenderer::~VulkanRenderer()
	{
		VK_CHECK(vkDeviceWaitIdle(m_Device));

		vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
		vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);

		vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);

		for (auto& imageView : m_SwapchainImageViews)
			vkDestroyImageView(m_Device, imageView, nullptr);

		vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);

		for (auto& shaderModule : m_ShaderStages)
			vkDestroyShaderModule(m_Device, shaderModule.module, nullptr);

		vkDestroyDevice(m_Device, nullptr);

		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

#if defined(BRICKENGINE_DEBUG)
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
		BRICKENGINE_ASSERT(vkDestroyDebugUtilsMessengerEXT);
		vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
#endif

		vkDestroyInstance(m_Instance, nullptr);
	}

	void VulkanRenderer::CreateInstance(std::vector<const char*>& requiredExtentions)
	{
		VkApplicationInfo applicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
		applicationInfo.apiVersion = VK_API_VERSION_1_1;
		applicationInfo.pEngineName = "BrickEngine";
		applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		applicationInfo.pApplicationName = "BrickEngine Application";
		applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);

		VkInstanceCreateInfo instanceCreateInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		instanceCreateInfo.pApplicationInfo = &applicationInfo;

#if defined(BRICKENGINE_DEBUG)
		std::vector<const char*> debugLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

		uint32_t availableLayerCount = 0;
		VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr));
		std::vector<VkLayerProperties> availableLayers(availableLayerCount);
		VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data()));

		bool hasRequiredLayers = [&]()
		{
			for (auto& requiredLayer : debugLayers)
			{
				bool hasLayer = false;
				for (auto& layer : availableLayers)
				{
					if (strcmp(layer.layerName, requiredLayer) == 0)
					{
						hasLayer = true;
						break;
					}
				}
				if (!hasLayer)
					return false;
			}
			return true;
		}();
		BRICKENGINE_ASSERT(hasRequiredLayers);

		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(debugLayers.size());
		instanceCreateInfo.ppEnabledLayerNames = debugLayers.data();
#endif

#if defined(BRICKENGINE_DEBUG)
		requiredExtentions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

		bool hasRequiredExtentions = [&]()
		{
			uint32_t availableExtentionCount = 0;
			VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &availableExtentionCount, nullptr));
			std::vector<VkExtensionProperties> availableExtentions(availableExtentionCount);
			VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &availableExtentionCount, availableExtentions.data()));

			for (auto& requiredExtention : requiredExtentions)
			{
				bool hasExtention = false;
				for (auto& extention : availableExtentions)
				{
					if (strcmp(extention.extensionName, requiredExtention) == 0)
					{
						hasExtention = true;
						break;
					}
				}
				if (!hasExtention)
					return false;
			}
			return true;
		}();
		BRICKENGINE_ASSERT(hasRequiredExtentions);

		instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtentions.size());
		instanceCreateInfo.ppEnabledExtensionNames = requiredExtentions.data();

		VK_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &m_Instance));
		BRICKENGINE_ASSERT(m_Instance);

#if defined(BRICKENGINE_DEBUG)
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;// | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
		debugCreateInfo.pfnUserCallback = VulkanDebugCallback;
		debugCreateInfo.pUserData = this;

		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
		BRICKENGINE_ASSERT(vkCreateDebugUtilsMessengerEXT);
		vkCreateDebugUtilsMessengerEXT(m_Instance, &debugCreateInfo, nullptr, &m_DebugMessenger);
		BRICKENGINE_ASSERT(m_DebugMessenger);
#endif
	}

	void VulkanRenderer::SelectPhysicalDevice(std::vector<const char*>& requiredExtentions)
	{
		uint32_t physicalDeviceCount = 0;
		VK_CHECK(vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, nullptr));
		std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
		VK_CHECK(vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, physicalDevices.data()));

		for (auto& physicalDevice : physicalDevices)
		{
			bool hasRequiredExtentions = [&]()
			{
				uint32_t physicalDeviceExtentionCount = 0;
				VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &physicalDeviceExtentionCount, nullptr));
				std::vector<VkExtensionProperties> physicalDeviceExtentions(physicalDeviceExtentionCount);
				VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &physicalDeviceExtentionCount, physicalDeviceExtentions.data()));

				for (auto& requiredExtention : requiredExtentions)
				{
					bool hasExtention = false;
					for (auto& extention : physicalDeviceExtentions)
					{
						if (strcmp(extention.extensionName, requiredExtention) == 0)
						{
							hasExtention = true;
							break;
						}
					}
					if (!hasExtention)
						return false;
				}
				return true;
			}();

			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
			std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

			uint32_t graphicsQueueFamilyIndex = [&]() -> uint32_t
			{
				for (uint32_t i = 0; i < queueFamilyCount; i++)
				{
					if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
						return i;
				}
				return -1;
			}();

			uint32_t presentQueueFamilyIndex = [&]() -> uint32_t
			{
				for (uint32_t i = 0; i < queueFamilyCount; i++)
				{
					VkBool32 supportsPresentation = VK_FALSE;
					VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, m_Surface, &supportsPresentation));
					if (supportsPresentation)
						return i;
				}
				return -1;
			}();

			uint32_t surfaceFormatCount = 0;
			VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &surfaceFormatCount, nullptr));
			std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
			VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &surfaceFormatCount, surfaceFormats.data()));

			VkSurfaceFormatKHR surfaceFormat = {};
			if (surfaceFormatCount == 0 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
				surfaceFormat = { VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
			else
			{
				for (auto& format : surfaceFormats)
				{
					if (format.format == VK_FORMAT_R8G8B8A8_UNORM && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
						surfaceFormat = format;
				}
			}

			uint32_t presentModeCount = 0;
			VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &presentModeCount, nullptr));
			std::vector<VkPresentModeKHR> presentModes(presentModeCount);
			VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &presentModeCount, presentModes.data()));

			VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
			for (auto& mode : presentModes)
			{
				if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
					presentMode = mode;
			}

			VkPhysicalDeviceProperties physicalDeviceProperties;
			vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

			VkPhysicalDeviceFeatures physicalDeviceFeatures;
			vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);

			if (
				hasRequiredExtentions																			&&
				graphicsQueueFamilyIndex < queueFamilyCount														&&
				presentQueueFamilyIndex < queueFamilyCount														&&
				surfaceFormat.format != VK_FORMAT_UNDEFINED														&&
				physicalDeviceFeatures.samplerAnisotropy														&&
				VK_VERSION_MAJOR(physicalDeviceProperties.apiVersion) >= VK_VERSION_MAJOR(VK_API_VERSION_1_1)	&&
				VK_VERSION_MINOR(physicalDeviceProperties.apiVersion) >= VK_VERSION_MINOR(VK_API_VERSION_1_1)	&&
				VK_VERSION_PATCH(physicalDeviceProperties.apiVersion) >= VK_VERSION_PATCH(VK_API_VERSION_1_1)
			)
			{
				m_PhysicalDevice = physicalDevice;
				m_GraphicsQueueFamilyIndex = graphicsQueueFamilyIndex;
				m_PresentQueueFamilyIndex = presentQueueFamilyIndex;
				m_SurfaceFormat = surfaceFormat;
				m_PresentMode = presentMode;
				if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
					break;
			}
		}
	}

	void VulkanRenderer::CreateDevice(std::vector<const char*>& requiredExtentions)
	{
		float queuePriorities[] = { 1.0f };
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		VkDeviceQueueCreateInfo& graphicsQueueCreateInfo = queueCreateInfos.emplace_back();
		graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		graphicsQueueCreateInfo.pQueuePriorities = queuePriorities;
		graphicsQueueCreateInfo.queueCount = 1;
		graphicsQueueCreateInfo.queueFamilyIndex = m_GraphicsQueueFamilyIndex;

		if (m_PresentQueueFamilyIndex != m_GraphicsQueueFamilyIndex)
		{
			VkDeviceQueueCreateInfo& presentQueueCreateInfo = queueCreateInfos.emplace_back();
			presentQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			presentQueueCreateInfo.pQueuePriorities = queuePriorities;
			presentQueueCreateInfo.queueCount = 1;
			presentQueueCreateInfo.queueFamilyIndex = m_PresentQueueFamilyIndex;
		}

		VkPhysicalDeviceFeatures physicalDeviceFeatures = {};
		physicalDeviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo deviceCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtentions.size());
		deviceCreateInfo.ppEnabledExtensionNames = requiredExtentions.data();
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

		VK_CHECK(vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device));
	}

	void VulkanRenderer::CreateShader(const std::string& path)
	{
		// Vertex Shader
		std::vector<char> vertexShaderSource = File::LoadFile(path + ".vert.spv");

		VkShaderModuleCreateInfo vertexShaderCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		vertexShaderCreateInfo.codeSize = vertexShaderSource.size() * sizeof(char);
		vertexShaderCreateInfo.pCode = (uint32_t*)vertexShaderSource.data();

		VkShaderModule vertexShaderModule;
		VK_CHECK(vkCreateShaderModule(m_Device, &vertexShaderCreateInfo, nullptr, &vertexShaderModule));

		VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		vertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderStageCreateInfo.module = vertexShaderModule;
		vertexShaderStageCreateInfo.pName = "main";

		m_ShaderStages.push_back(vertexShaderStageCreateInfo);

		// Fragment Shader
		std::vector<char> fragmentShaderSource = File::LoadFile(path + ".frag.spv");

		VkShaderModuleCreateInfo fragmentShaderCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		fragmentShaderCreateInfo.codeSize = fragmentShaderSource.size() * sizeof(char);
		fragmentShaderCreateInfo.pCode = (uint32_t*)fragmentShaderSource.data();

		VkShaderModule fragmentShaderModule;
		VK_CHECK(vkCreateShaderModule(m_Device, &fragmentShaderCreateInfo, nullptr, &fragmentShaderModule));

		VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		fragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderStageCreateInfo.module = fragmentShaderModule;
		fragmentShaderStageCreateInfo.pName = "main";

		m_ShaderStages.push_back(fragmentShaderStageCreateInfo);
	}

	void VulkanRenderer::OnWindowResize()
	{
		CreateSwapchain();
		BRICKENGINE_ASSERT(m_Swapchain);

		CreateSwapchainImagesAndViews();
	}

	void VulkanRenderer::CreateSwapchain()
	{
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &surfaceCapabilities));

		if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			m_SwapchainExtent = surfaceCapabilities.currentExtent;
		else
		{
			m_SwapchainExtent = { (uint32_t)m_Window->GetWidth(), (uint32_t)m_Window->GetHeight() };
			m_SwapchainExtent.width = std::clamp(m_SwapchainExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
			m_SwapchainExtent.height = std::clamp(m_SwapchainExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
		}

		uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
		imageCount = std::min(imageCount, surfaceCapabilities.maxImageCount);

		VkSwapchainCreateInfoKHR swapchainCreateInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		swapchainCreateInfo.surface = m_Surface;
		swapchainCreateInfo.minImageCount = imageCount;
		swapchainCreateInfo.imageFormat = m_SurfaceFormat.format;
		swapchainCreateInfo.imageColorSpace = m_SurfaceFormat.colorSpace;
		swapchainCreateInfo.imageExtent = m_SwapchainExtent;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		uint32_t queueFamilyIndices[] = { m_GraphicsQueueFamilyIndex, m_PresentQueueFamilyIndex };
		if (m_GraphicsQueueFamilyIndex != m_PresentQueueFamilyIndex)
		{
			swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapchainCreateInfo.queueFamilyIndexCount = 2;
			swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapchainCreateInfo.queueFamilyIndexCount = 0;
			swapchainCreateInfo.pQueueFamilyIndices = nullptr;
		}

		VkSwapchainKHR oldSwapchain = m_Swapchain;

		swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCreateInfo.presentMode = m_PresentMode;
		swapchainCreateInfo.clipped = VK_TRUE;
		swapchainCreateInfo.oldSwapchain = oldSwapchain;

		VK_CHECK(vkCreateSwapchainKHR(m_Device, &swapchainCreateInfo, nullptr, &m_Swapchain));

		if (oldSwapchain)
			vkDestroySwapchainKHR(m_Device, oldSwapchain, nullptr);
	}

	void VulkanRenderer::CreateSwapchainImagesAndViews()
	{
		m_SwapchainImages.clear();
		if (m_SwapchainImageViews.size() > 0)
			for (auto& imageView : m_SwapchainImageViews)
				vkDestroyImageView(m_Device, imageView, nullptr);
		m_SwapchainImageViews.clear();

		uint32_t swapchainImageCount = 0;
		VK_CHECK(vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &swapchainImageCount, nullptr));
		m_SwapchainImages.resize(swapchainImageCount);
		VK_CHECK(vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &swapchainImageCount, m_SwapchainImages.data()));

		m_SwapchainImageViews.resize(swapchainImageCount);
		for (uint32_t i = 0; i < swapchainImageCount; i++)
		{
			VkImageViewCreateInfo imageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			imageViewCreateInfo.image = m_SwapchainImages[i];
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.format = m_SurfaceFormat.format;
			imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 1;

			VK_CHECK(vkCreateImageView(m_Device, &imageViewCreateInfo, nullptr, &m_SwapchainImageViews[i]));
		}
	}

	void VulkanRenderer::CreateRenderPass()
	{
		// Color Attachment
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = m_SurfaceFormat.format;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRefrence = {};
		colorAttachmentRefrence.attachment = 0;
		colorAttachmentRefrence.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// Depth Attachment
		std::vector<VkFormat> depthFormatCandidates = {
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT
		};
		VkFormat depthFormat = [&]()
		{
			uint32_t flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
			for (auto& candidate : depthFormatCandidates)
			{
				VkFormatProperties properties;
				vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, candidate, &properties);

				if ((properties.linearTilingFeatures & flags) == flags)
					return candidate;
				else if ((properties.optimalTilingFeatures & flags) == flags)
					return candidate;
			}
			BRICKENGINE_ASSERT(false && "Unable to find suitable depth format!");
			return VK_FORMAT_UNDEFINED;
		}();

		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = depthFormat;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRefrence = {};
		depthAttachmentRefrence.attachment = 1;
		depthAttachmentRefrence.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// Subpass
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRefrence;
		subpass.pDepthStencilAttachment = &depthAttachmentRefrence;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		std::vector<VkAttachmentDescription> attachments = {
			colorAttachment,
			depthAttachment
		};

		VkRenderPassCreateInfo renderPassCreatInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		renderPassCreatInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassCreatInfo.pAttachments = attachments.data();
		renderPassCreatInfo.subpassCount = 1;
		renderPassCreatInfo.pSubpasses = &subpass;
		renderPassCreatInfo.dependencyCount = 1;
		renderPassCreatInfo.pDependencies = &dependency;

		VK_CHECK(vkCreateRenderPass(m_Device, &renderPassCreatInfo, nullptr, &m_RenderPass));
	}

	void VulkanRenderer::CreateGraphicsPipeline()
	{
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = static_cast<float>(m_SwapchainExtent.height);
		viewport.width = static_cast<float>(m_SwapchainExtent.width);
		viewport.height = -static_cast<float>(m_SwapchainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = { m_SwapchainExtent.width, m_SwapchainExtent.height };

		VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
		rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
		rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizerCreateInfo.lineWidth = 1.0f;
		rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
		rasterizerCreateInfo.depthBiasConstantFactor = 0.0f;
		rasterizerCreateInfo.depthBiasClamp = 0.0f;
		rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
		multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
		multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisamplingCreateInfo.minSampleShading = 1.0f;
		multisamplingCreateInfo.pSampleMask = nullptr;
		multisamplingCreateInfo.alphaToCoverageEnable = VK_FALSE;
		multisamplingCreateInfo.alphaToOneEnable = VK_FALSE;

		VkPipelineDepthStencilStateCreateInfo depthStencil = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
		colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachmentState.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
		colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
		colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
		colorBlendStateCreateInfo.attachmentCount = 1;
		colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
		colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
		colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
		colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
		colorBlendStateCreateInfo.blendConstants[3] = 0.0f;

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};

		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
		dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

		VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
		vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		pipelineLayoutCreateInfo.setLayoutCount = 0;
		pipelineLayoutCreateInfo.pSetLayouts = nullptr;
		pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

		VK_CHECK(vkCreatePipelineLayout(m_Device, &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout));

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(m_ShaderStages.size());
		pipelineCreateInfo.pStages = m_ShaderStages.data();
		pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
		pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
		pipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
		pipelineCreateInfo.pDepthStencilState = &depthStencil;
		pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
		pipelineCreateInfo.pDynamicState = nullptr;

		pipelineCreateInfo.layout = m_PipelineLayout;
		pipelineCreateInfo.renderPass = m_RenderPass;
		pipelineCreateInfo.subpass = 0;
		pipelineCreateInfo.basePipelineHandle = nullptr;
		pipelineCreateInfo.basePipelineIndex = -1;

		VK_CHECK(vkCreateGraphicsPipelines(m_Device, nullptr, 1, &pipelineCreateInfo, nullptr, &m_Pipeline));
	}

}
