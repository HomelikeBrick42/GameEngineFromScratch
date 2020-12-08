#pragma once

#include "BrickEngine/Core/Base.hpp"

namespace BrickEngine {

	class BRICKENGINE_API Log
	{
	public:
		Log() = delete;

		template<typename... Args>
		BRICKENGINE_FORCE_INLINE static void Trace(const std::string& message)
		{
			std::cout << "[TRACE]: " << message << std::endl;
		}

		template<typename... Args>
		BRICKENGINE_FORCE_INLINE static void Info(const std::string& message)
		{
			std::cout << "[INFO]: " << message << std::endl;
		}

		template<typename... Args>
		BRICKENGINE_FORCE_INLINE static void Warn(const std::string& message)
		{
			std::cout << "[WARN]: " << message << std::endl;
		}

		template<typename... Args>
		BRICKENGINE_FORCE_INLINE static void Error(const std::string& message)
		{
			std::cout << "[ERROR]: " << message << std::endl;
		}

		template<typename... Args>
		BRICKENGINE_FORCE_INLINE static void Fatal(const std::string& message)
		{
			std::cout << "[FATAL]: " << message << std::endl;
		}
	};

}
