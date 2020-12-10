#pragma once

#include "BrickEngine/Core/Window.hpp"

#if defined(BRICKENGINE_PLATFORM_WINDOWS)

#include <Windows.h>

namespace BrickEngine {

	class WindowsWindow final : public Window
	{
		friend class VulkanPlatform;
	public:
		WindowsWindow(uint32_t width, uint32_t height, const std::string& title, bool resizable);
		~WindowsWindow();

		virtual bool WantsToClose() override final { return m_WantsToClose; }
		virtual void PollEvents() override final;

		virtual int32_t GetWidth() override final { return m_Width; }
		virtual int32_t GetHeight() override final { return m_Height; }
	private:
		static LRESULT CALLBACK HandleMessagesStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		LRESULT CALLBACK HandleMessages(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	private:
		HINSTANCE m_Instance;
		HWND m_HWND;
		HDC m_DC;
		bool m_WantsToClose = false;
		int32_t m_Width;
		int32_t m_Height;
	};

}

#endif
