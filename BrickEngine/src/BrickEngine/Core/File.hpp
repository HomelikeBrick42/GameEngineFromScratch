#pragma once

#include "BrickEngine/Core/Base.hpp"

namespace BrickEngine {

	class File
	{
	public:
		File() = delete;

		static std::vector<char> LoadFile(const std::string& filepath);
	};

}
