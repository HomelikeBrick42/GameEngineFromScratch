
%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=vert %~dp0\..\Sandbox\assets\shaders\main.vert.glsl -o %~dp0\..\Sandbox\assets\shaders\main.vert.spv
%VULKAN_SDK%\Bin\glslc.exe -fshader-stage=frag %~dp0\..\Sandbox\assets\shaders\main.frag.glsl -o %~dp0\..\Sandbox\assets\shaders\main.frag.spv
pause
