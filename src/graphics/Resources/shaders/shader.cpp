#include"shader.h"

namespace CRATER::Resource {

    Shader::Shader(const std::string& id,
        vk::raii::Device* device,
        Renderer::VulkanSwapChain* swapChain,
        vk::Format format,
        PipelineType pipelineType,
        const std::string& shaderPath,
        const std::string& vertEntryPoint,
        const std::string& fragEntryPoint)
        :Resource(id),
        m_device(device),
        m_swapChain(swapChain),
        m_format(format),
        m_pipelineType(pipelineType),
        m_vertEntryPoint(vertEntryPoint),
        m_fragEntryPoint(fragEntryPoint)
    {
        std::string outputPath = "temp_shader.spv";

        // Build slangc command
        std::ostringstream command;
        command << "slangc "
            << shaderPath
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

        m_spirvCode = spirvCode;


    }

    bool Shader::doLoad() {
        try {
            m_descriptorSetLayout.reflectShader(m_spirvCode);
            m_descriptorSetLayout.create(*m_device);
            m_pushConstant.reflectShader(m_spirvCode);

            m_pipelineLayout.create(*m_device, m_descriptorSetLayout, m_pushConstant);

            m_graphicsPipeline.create(
                m_pipelineLayout,
                *m_device,
                (*m_swapChain).extent(),
                (*m_swapChain).format(),
                (*m_swapChain).surfaceFormat(),
                m_spirvCode,
                m_format,
                m_pipelineType
            );

            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[Shader] doLoad failed  " << e.what() << "\n";//std::cerr << "[Shader] doLoad failed (" << m_shaderPath << "): " << e.what() << "\n";
            
            return false;
        }
    }

    void Shader::doUnload() {
       // m_graphicsPipeline=std::move(Renderer::GraphicsPipeline{});
       // m_pipelineLayout = Renderer::PipelineLayout{};
       // m_pushConstant = Renderer::PushConstant{};
        m_descriptorSetLayout = Renderer::DescriptorSetLayout{};
    }



	 
	 
}