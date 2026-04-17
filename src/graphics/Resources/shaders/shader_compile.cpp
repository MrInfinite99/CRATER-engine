#include"shader_compile.h"

namespace CRATER::ResourceManager {
	std::vector<char> ShaderCompiler::compileShader(
        const std::string& shaderPath,
        const std::string& vertEntryPoint,
        const std::string& fragEntryPoint
    ){
        std::string outputPath = "temp_shader.spv";

        // Build slangc command
        std::ostringstream command;
        command << "slangc "
            <<    shaderPath
            << " -target spirv"
            << " -profile spirv_1_4"
            << " -emit-spirv-directly"
            << " -fvk-use-entrypoint-name"
            << " -entry " << vertEntryPoint
            << " -entry " << fragEntryPoint
            << " -o " << outputPath;

        // Execute compiler
        int result = std::system(command.str().c_str());
        if (result != 0) {
            throw std::runtime_error("Failed to compile shader: " + shaderPath);
        }

        // Read compiled SPIR-V file
        std::ifstream file(outputPath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to read compiled shader");
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> spirvCode(size);
        if (!file.read(spirvCode.data(), size)) {
            throw std::runtime_error("Failed to read SPIR-V data");
        }

        file.close();

        // Cleanup temp file
        std::remove(outputPath.c_str());

        return spirvCode;
	}
	 
}