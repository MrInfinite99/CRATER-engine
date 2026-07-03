#include"shader.h"
#include"embedded_shaders.h"

namespace CRATER::Resource {

    Shader::Shader(const std::string& id,
        vk::raii::Device* device,
        Renderer::VulkanSwapChain* swapChain,
        vk::Format format,
        PipelineType pipelineType,
        slang::IGlobalSession* slangSession,
        const std::string& shaderPath,
        const std::string& vertEntryPoint,
        const std::string& fragEntryPoint)
        :Resource(id),
        m_device(device),
        m_swapChain(swapChain),
        m_format(format),
        m_pipelineType(pipelineType),
        m_slangSession(slangSession),
        m_shaderPath(shaderPath),
        m_vertEntryPoint(vertEntryPoint),
        m_fragEntryPoint(fragEntryPoint)
    {
       

    }

    bool Shader::doLoad() {
        try {
            slang::TargetDesc target{};
            target.format = SLANG_SPIRV;
            target.profile = m_slangSession->findProfile("spirv_1_4");
            target.flags = SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY;

            auto dir = std::filesystem::path(m_shaderPath).parent_path().string();
            const char* searchPaths[] = { dir.c_str() };

            slang::SessionDesc sessionDesc{};
            sessionDesc.targets = &target;
            sessionDesc.targetCount = 1;
            sessionDesc.searchPaths = searchPaths;
            sessionDesc.searchPathCount = 1;

            Slang::ComPtr<slang::ISession> session;
            m_slangSession->createSession(sessionDesc, session.writeRef());

            // loadModule takes just the stem: "basic" finds "basic.slang"
            auto moduleName = std::filesystem::path(m_shaderPath).stem().string();

            Slang::ComPtr<ISlangBlob> diagnostics;
            slang::IModule* mod = session->loadModule(moduleName.c_str(), diagnostics.writeRef());
            if (diagnostics && diagnostics->getBufferSize() > 0)
                std::cerr << "[Shader] " << (const char*)diagnostics->getBufferPointer();

            if (!mod) {
                std::cerr << "[Shader] File load failed for " << m_shaderPath
                          << " — compiling embedded fallback\n";
                diagnostics = nullptr;
                mod = session->loadModuleFromSourceString(
                    moduleName.c_str(),
                    m_shaderPath.c_str(),
                    kEmbeddedBasicShader,
                    diagnostics.writeRef()
                );
                if (diagnostics && diagnostics->getBufferSize() > 0)
                    std::cerr << "[Shader] Embedded: " << (const char*)diagnostics->getBufferPointer();
                if (!mod)
                    throw std::runtime_error("Embedded fallback shader also failed: " + m_shaderPath);
            }

            Slang::ComPtr<slang::IEntryPoint> vertEntry, fragEntry;
            mod->findEntryPointByName(m_vertEntryPoint.c_str(), vertEntry.writeRef());
            mod->findEntryPointByName(m_fragEntryPoint.c_str(), fragEntry.writeRef());

            slang::IComponentType* parts[] = { mod, vertEntry, fragEntry };
            Slang::ComPtr<slang::IComponentType> composed;
            session->createCompositeComponentType(parts, 3, composed.writeRef(), diagnostics.writeRef());

            Slang::ComPtr<slang::IComponentType> linked;
            composed->link(linked.writeRef(), diagnostics.writeRef());

            Slang::ComPtr<ISlangBlob> spirv;
            linked->getTargetCode(0, spirv.writeRef(), diagnostics.writeRef());
            if (!spirv)
                throw std::runtime_error("Failed to get SPIR-V: " + m_shaderPath);

            auto* data = static_cast<const char*>(spirv->getBufferPointer());
            m_spirvCode.assign(data, data + spirv->getBufferSize());



            m_descriptorSetLayout.reflectShader(m_spirvCode);
            m_descriptorSetLayout.create(*m_device);
            m_pushConstant.reflectShader(m_spirvCode);

            m_pipelineLayout.create(*m_device, m_descriptorSetLayout, m_pushConstant);

            m_graphicsPipeline.create(
                m_pipelineLayout,
                *m_device,
                m_swapChain->extent(),
                m_swapChain->format(),
                m_swapChain->surfaceFormat(),
                m_spirvCode,
                m_format,
                m_pipelineType
            );

            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[Shader] doLoad failed  " << e.what() << "\n";
            
            return false;
        }
    }

    void Shader::doUnload() {
        //m_graphicsPipeline=Renderer::GraphicsPipeline{};
       // m_pipelineLayout = Renderer::PipelineLayout{};
       // m_pushConstant = Renderer::PushConstant{};
        m_descriptorSetLayout = Renderer::DescriptorSetLayout{};
        m_spirvCode.clear();
    }



	 
	 
}