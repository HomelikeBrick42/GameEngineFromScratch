#include "brickpch.hpp"
#include "BrickEngine/Renderer/Vulkan/VulkanRenderer.hpp"

#define VK_CHECK(x) { \
	VkResult result = x; BRICKENGINE_ASSERT(result == VK_SUCCESS) \
}

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

		VkDeviceCreateInfo deviceCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtentions.size());
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtentions.data();
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

		VK_CHECK(vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device));
		BRICKENGINE_ASSERT(m_Device);
	}

	VulkanRenderer::~VulkanRenderer()
	{
		vkDeviceWaitIdle(m_Device);
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

			VkSurfaceCapabilitiesKHR surfaceCapabilities;
			VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_Surface, &surfaceCapabilities));

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
				m_SurfaceCapabilities = surfaceCapabilities;
				m_SurfaceFormat = surfaceFormat;
				m_PresentMode = presentMode;
				if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
					break;
			}
		}
	}

}
