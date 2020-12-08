#include "brickpch.hpp"
#include "Platform/Windows/WindowsWindow.hpp"

#if defined(BRICKENGINE_PLATFORM_WINDOWS)

namespace BrickEngine {

	constexpr LPCWSTR s_ClassName = L"BrickEngineWindowClass";

	static std::uint32_t s_WindowCount = 0;

	WindowsWindow::WindowsWindow(uint32_t width, uint32_t height, const std::string& title)
		: m_Instance(GetModuleHandle(nullptr))
	{
		if (s_WindowCount == 0)
		{
			WNDCLASSEX wc = {};
			wc.cbSize = sizeof(WNDCLASSEX);
			wc.style = CS_OWNDC;
			wc.lpfnWndProc = static_cast<LRESULT(CALLBACK*)(HWND, UINT, WPARAM, LPARAM)>([](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT
				{
					if (msg == WM_NCCREATE)
					{
						CREATESTRUCT* create = reinterpret_cast<CREATESTRUCT*>(lParam);
						WindowsWindow* window = static_cast<WindowsWindow*>(create->lpCreateParams);
						SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
						SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WindowsWindow::HandleMessagesStatic));
						return window->HandleMessages(hWnd, msg, wParam, lParam);
					}

					return DefWindowProc(hWnd, msg, wParam, lParam);
				}
			);
			wc.cbClsExtra = 0;
			wc.cbWndExtra = 0;
			wc.hIcon = LoadIcon(m_Instance, IDI_APPLICATION);
			wc.hIconSm = LoadIcon(m_Instance, IDI_APPLICATION);
			wc.hCursor = LoadCursor(NULL, IDC_ARROW);
			wc.hbrBackground = nullptr;
			wc.lpszMenuName = nullptr;
			wc.lpszClassName = s_ClassName;
			wc.hIconSm = nullptr;
			RegisterClassEx(&wc);
		}
		s_WindowCount++;

		RECT wr = {};
		wr.left = 100;
		wr.right = width + wr.left;
		wr.top = 100;
		wr.bottom = height + wr.top;
		AdjustWindowRect(&wr, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU, false);

		std::wstring stemp(title.begin(), title.end());

		m_HWND = CreateWindow(
			s_ClassName,
			stemp.c_str(),
			WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
			CW_USEDEFAULT, CW_USEDEFAULT,
			wr.right - wr.left,
			wr.bottom - wr.top,
			nullptr,
			nullptr,
			m_Instance,
			this
		);

		m_DC = GetDC(m_HWND);

		ShowWindow(m_HWND, SW_SHOWDEFAULT);
	}

	WindowsWindow::~WindowsWindow()
	{
		ReleaseDC(m_HWND, m_DC);
		DestroyWindow(m_HWND);
		s_WindowCount--;
		if (s_WindowCount == 0)
			UnregisterClass(s_ClassName, m_Instance);
	}

	void WindowsWindow::PollEvents()
	{
		MSG msg;
		while (PeekMessage(&msg, m_HWND, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	LRESULT CALLBACK WindowsWindow::HandleMessagesStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		WindowsWindow* window = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		return window->HandleMessages(hWnd, msg, wParam, lParam);
	}

	LRESULT CALLBACK WindowsWindow::HandleMessages(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
			case WM_QUIT:
			case WM_CLOSE:
			{
				m_WantsToClose = true;
				return 0;
			}
		}

		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

}

#endif
