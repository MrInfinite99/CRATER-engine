#pragma once

#include "pipeline_registry.h"




namespace CRATER::Renderer {
	class PipelineLayout {
	private:
		vk::raii::PipelineLayout m_pipelineLayout{ nullptr };
		

		
	public:
		 PipelineLayout() = default;
		~PipelineLayout() = default;

		 
		void create(vk::raii::Device& device, DescriptorSetLayout& descriptorSetLayout, PushConstant& pushConstant);

		vk::raii::PipelineLayout& layout() {
			return m_pipelineLayout;
		}
	};
	//For a completely generic pipeline create a config struct ,to support different kinds of pipelines
	 

	class GraphicsPipeline {
	public:
		GraphicsPipeline() = default;
		~GraphicsPipeline() = default;
 
		void create(PipelineLayout& layout, vk::raii::Device& device, vk::Extent2D& swapChainExtent, vk::Format& swapChainImageFormat, vk::SurfaceFormatKHR   swapChainSurfaceFormat, const std::vector<char>& code, vk::Format depthFormat, PipelineType type);
	     const vk::raii::Pipeline& pipeline() const  {
			return m_graphicsPipeline;  // No dereference needed
		}
 
	private:
		vk::raii::ShaderModule m_shaderModule{ nullptr };
		vk::raii::ShaderModule createShaderModule(const std::vector<char>& code, vk::raii::Device& device) const;
		vk::raii::Pipeline m_graphicsPipeline{ nullptr };
		PipelineConfig config;

		 
	};
}

 