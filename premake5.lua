workspace "GameEngineFromScratch"
	architecture "x64"
	startproject "Sandbox"

	configurations
	{
		"Debug",
		"Release"
	}
	
	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
	
project "BrickEngine"
	location "BrickEngine"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	
	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "brickpch.hpp"
	pchsource "%{prj.name}/src/brickpch.cpp"
	
	files
	{
		"%{wks.location}/%{prj.name}/src/**.hpp",
		"%{wks.location}/%{prj.name}/src/**.cpp"
	}
	
	includedirs
	{
		"%{wks.location}/%{prj.name}/src"
	}
	
	links
	{
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"BRICKENGINE_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "BRICKENGINE_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "BRICKENGINE_RELEASE"
		runtime "Release"
		optimize "on"
		
project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	
	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")
	
	pchheader "pch.hpp"
	pchsource "%{prj.name}/src/pch.cpp"

	files
	{
		"%{wks.location}/%{prj.name}/src/**.hpp",
		"%{wks.location}/%{prj.name}/src/**.cpp"
	}
	
	includedirs
	{
		"%{wks.location}/%{prj.name}/src",
		"%{wks.location}/BrickEngine/src"
	}

	links
	{
		"BrickEngine"
	}

	defines
	{
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"BRICKENGINE_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "BRICKENGINE_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "BRICKENGINE_RELEASE"
		runtime "Release"
		optimize "on"
