#pragma once

#include "BrickEngine/Core/Base.hpp"

namespace BrickEngine {

	class BRICKENGINE_API Window
	{
	public:
		virtual ~Window() = default;

		virtual bool WantsToClose() = 0;
		virtual void PollEvents() = 0;

		static Window* Create(uint32_t width, uint32_t height, const std::string& title);
	protected:
		Window() = default;
	};

}
