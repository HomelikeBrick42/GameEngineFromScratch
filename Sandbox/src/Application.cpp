#include "pch.hpp"
#include "Application.hpp"

using namespace BrickEngine;

void Application::Run()
{
	using namespace std::chrono;

	Init();
	high_resolution_clock::time_point time, lastTime = high_resolution_clock::now();
	double delta;
	while (!m_Window->WantsToClose())
	{
		time = high_resolution_clock::now();
		delta = duration<double>(time - lastTime).count();
		lastTime = high_resolution_clock::now();
		Update(delta);
	}
	Shutdown();
}

void Application::Init()
{
	m_Window.reset(Window::Create(1280, 720, "Vulkan Engine"));
	m_Renderer.reset(new VulkanRenderer(m_Window.get()));
}

void Application::Update(const double& dt)
{
	m_Window->PollEvents();
}

void Application::Shutdown()
{
}
