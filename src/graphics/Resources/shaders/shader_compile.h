#pragma once
#include<vector>
#include<string>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstdio>

namespace CRATER::ResourceManager {
	class ShaderCompiler {
	public:
		ShaderCompiler() = default;
		~ShaderCompiler() = default;


		std::vector<char> compileShader(
			const std::string& shaderPath,
			const std::string& vertEntryPoint,
			const std::string& fragEntryPoint
		);
	};
}