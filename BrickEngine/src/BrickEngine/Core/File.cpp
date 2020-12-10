#include "brickpch.hpp"
#include "BrickEngine/Core/File.hpp"

namespace BrickEngine {

	std::vector<char> File::LoadFile(const std::string& filepath)
	{
		std::ifstream file(filepath, std::ios::ate | std::ios::binary);
		BRICKENGINE_ASSERT(file.is_open());
		std::vector<char> data(file.tellg());
		file.seekg(0);
		file.read(data.data(), data.size() * sizeof(char));
		file.close();
		return data;
	}

}
