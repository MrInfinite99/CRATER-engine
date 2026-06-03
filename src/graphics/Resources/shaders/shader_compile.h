#pragma once
#include<vector>
#include<string>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstdio>

namespace CRATER::Resource {
	class ShaderCompiler {
	private:
		ShaderCompiler(){}
	public:
		ShaderCompiler(const ShaderCompiler&) = delete;
		ShaderCompiler& operator=(const ShaderCompiler&) = delete;

		static ShaderCompiler& get() {
			static ShaderCompiler instance;
			return instance;
		}

		std::vector<char> compileShader(
			const std::string& shaderPath,
			const std::string& vertEntryPoint,
			const std::string& fragEntryPoint
		);
	};
}