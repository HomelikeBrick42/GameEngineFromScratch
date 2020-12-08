#pragma once

#include "pch.hpp"

#include "BrickEngine/Renderer/Vulkan/VulkanRenderer.hpp"

class Application
{
public:
	void Run();
private:
	void Init();
	void Update(const double& dt);
	void Shutdown();
private:
	std::unique_ptr<BrickEngine::Window> m_Window = nullptr;
	std::unique_ptr<BrickEngine::VulkanRenderer> m_Renderer = nullptr; // TEMPORARY
};
