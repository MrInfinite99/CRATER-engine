#pragma once
#include<vector>
#include<string>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstdio>
#include"../../../engineTypes.h"
#include"../../Renderer/graphics_pipeline.h"
#include"../../Renderer/descriptor_set.h"
#include"../../Renderer/Device.h"
#include"../../Renderer/swap_chain.h"
#include"../resource.h"


namespace CRATER::Resource {

	class Shader:public Resource {

	public:

		bool doLoad() override;
		void doUnload() override;

		explicit Shader(const std::string& id,
			vk::raii::Device* device,
			Renderer::VulkanSwapChain* swapChain,
			vk::Format format,
			PipelineType pipelineType,
			const std::string& shaderPath,
			const std::string& vertEntryPoint,
			const std::string& fragEntryPoint);
			
		


		Renderer::GraphicsPipeline& pipeline() { return m_graphicsPipeline; }
		Renderer::PipelineLayout& layout() { return m_pipelineLayout; }
		Renderer::PushConstant& pushConstant() { return m_pushConstant; }
		Renderer::DescriptorSetLayout& descriptorSetLayout() { return m_descriptorSetLayout; }

		

	private:
	 
		vk::raii::Device* m_device{};
		Renderer::VulkanSwapChain* m_swapChain;
		std::vector<char>          m_spirvCode;
		vk::Format                 m_format;
		PipelineType               m_pipelineType;
		std::string m_vertEntryPoint;
			std::string m_fragEntryPoint;
	 
		Renderer::PipelineLayout       m_pipelineLayout{};
		Renderer::PushConstant         m_pushConstant{};
		Renderer::GraphicsPipeline     m_graphicsPipeline{};
		Renderer::DescriptorSetLayout  m_descriptorSetLayout{};

 
	};

 
}