#pragma once
#include"../Renderer/graphics_pipeline.h"
#include"../Renderer/descriptor_set.h"
#include"../Renderer/Device.h"
#include"../Renderer/swap_chain.h"
 
 


namespace CRATER::Object {

	class Shader{
	 public:
		Renderer::PipelineLayout pipelineLayout{};
		Renderer::PushConstant pushConstant{};
		Renderer::GraphicsPipeline graphicsPipeline{};
		Renderer::DescriptorSetLayout descriptorSetLayout{};


		void init(vk::raii::Device& device,
			Renderer::VulkanSwapChain& swapChain,
			const std::vector<char>& shaderCode,
			vk::Format format,
			PipelineType pipelineType)
		{

			descriptorSetLayout.reflectShader(shaderCode);
			descriptorSetLayout.create(device);
			pushConstant.reflectShader(shaderCode);

			pipelineLayout.create(device, descriptorSetLayout, pushConstant);

			graphicsPipeline.create(
				pipelineLayout,
				device,
				swapChain.extent(),
				swapChain.format(),
				swapChain.surfaceFormat(),
				shaderCode,
				format,
				pipelineType
			);
		}

	};

	struct Material {
		Shader* shaderRef;
		Renderer::DescriptorSet descriptorSets{};
		

		void init(vk::raii::Device& device, Shader* shader) {
			shaderRef = shader;
			descriptorSets.create(device, &shaderRef->descriptorSetLayout);
		}

		template<typename T>
		void updateDescriptors(vk::raii::Device& device,
			uint32_t frameIndex,
			Resource::VulkanUniformBuffer<T>& uniformBuffer,
			Resource::Texture& texture)

		{
			vk::DescriptorBufferInfo bInfo{
				uniformBuffer[frameIndex].buffer(),
				0,
				sizeof(UniformBufferObject)
			};
			descriptorSets.updateBuffer(device, frameIndex, "ubo", bInfo);

			vk::DescriptorImageInfo iInfo{
				texture.sampler(),
				texture.view(),
				vk::ImageLayout::eShaderReadOnlyOptimal
			};
			descriptorSets.updateImage(device, frameIndex, "texture", iInfo);
		}

		 

		template<typename T>
		void updateDescriptors(vk::raii::Device& device,
			uint32_t frameIndex,
			Resource::VulkanUniformBuffer<T>& uniformBuffer,
			Resource::SkyboxTexture& texture)

		{
			vk::DescriptorBufferInfo bInfo{
				uniformBuffer[frameIndex].buffer(),
				0,
				sizeof(UniformBufferObject)
			};
			descriptorSets.updateBuffer(device, frameIndex, "ubo", bInfo);

			vk::DescriptorImageInfo iInfo{
				texture.sampler(),
				texture.view(),
				vk::ImageLayout::eShaderReadOnlyOptimal
			};
			descriptorSets.updateImage(device, frameIndex, "skyboxTexture", iInfo);
		}

	};

}