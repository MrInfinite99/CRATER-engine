#pragma once
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include"../Resources/shaders/shader_compile.h"
#include"../Resources/vertex_buffer.h"
#include"descriptor_set.h"


namespace CRATER::Renderer {
	class VulkanGraphicsPipeline {
	public:
		VulkanGraphicsPipeline() = default;
		~VulkanGraphicsPipeline() = default;

		 

		void createPipeline(vk::raii::Device& device, vk::Extent2D& swapChainExtent, vk::Format& swapChainImageFormat, vk::SurfaceFormatKHR   swapChainSurfaceFormat,const std::vector<char>& code, ResourceManager::VulkanVertexBuffer& vertex, DescriptorSet& descriptorSets,vk::Format depthFormat);
		 
		const vk::raii::Pipeline& pipeline() const {
			return m_graphicsPipeline;  // No dereference needed
		}

		 auto& layout() {
			return m_pipelineLayout;
		}

	private:
		
		vk::raii::ShaderModule createShaderModule(const std::vector<char>& code, vk::raii::Device& device) const;
		vk::raii::ShaderModule m_shaderModule{ nullptr };
		vk::raii::PipelineLayout m_pipelineLayout{ nullptr };
		vk::raii::Pipeline m_graphicsPipeline{ nullptr };

		 
	};
}

 