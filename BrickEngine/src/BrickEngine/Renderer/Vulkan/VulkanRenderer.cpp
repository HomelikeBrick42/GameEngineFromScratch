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
#if defined(BRICKENGINE_DEBUG)
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
			VK_KHR_SURFACE_EXTENSION_NAME,

#if defined(BRICKENGINE_PLATFORM_WINDOWS)
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#else
	#error Unsupported platform!
#endif
		};

		VkApplicationInfo applicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
		applicationInfo.apiVersion = VK_VERSION_1_2;
		applicationInfo.pEngineName = "BrickEngine";
		applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		applicationInfo.pApplicationName = "BrickEngine Application";
		applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);

		VkInstanceCreateInfo instanceCreateInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		instanceCreateInfo.pApplicationInfo = &applicationInfo;
		instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtentions.size());
		instanceCreateInfo.ppEnabledExtensionNames = instanceExtentions.data();

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

		VK_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &m_Instance));

#if defined(BRICKENGINE_DEBUG)
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
		debugCreateInfo.pfnUserCallback = VulkanDebugCallback;
		debugCreateInfo.pUserData = this;

		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
		BRICKENGINE_ASSERT(vkCreateDebugUtilsMessengerEXT);
		vkCreateDebugUtilsMessengerEXT(m_Instance, &debugCreateInfo, nullptr, &m_DebugMessenger);
#endif
	}

	VulkanRenderer::~VulkanRenderer()
	{
#if defined(BRICKENGINE_DEBUG)
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
		BRICKENGINE_ASSERT(vkDestroyDebugUtilsMessengerEXT);
		vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
#endif
		vkDestroyInstance(m_Instance, nullptr);
	}

}
