#pragma once
 
#include"../shaders/shader.h"
#include"../textures/texture.h"
 
 


namespace CRATER::Resource {

	 

	class Material :public Resource{
	private:

		Renderer::VulkanDevice* m_device;

		ResourceHandle<Shader>  m_shader;

		Renderer::DescriptorSet descriptorSets{};
	public:
		Material(const std::string& id,
			Renderer::VulkanDevice* device,
			ResourceHandle<Shader> shaderHandle):
			Resource(id),
			m_device(device),
			m_shader(std::move(shaderHandle))
		{ }

		Shader* getShader() {
			return m_shader.Get();
		}

		Renderer::DescriptorSet& getDescriptorSet() {
			return descriptorSets;
		 }

		bool doLoad() override {
			 
			descriptorSets.create(m_device->logicalDevice(),&m_shader->descriptorSetLayout());

			return true;
		}

		void doUnload() override {
			return;
		}

		template<typename T>
		void updateDescriptors(vk::raii::Device& device,
			uint32_t frameIndex,
			VulkanUniformBuffer<T>& uniformBuffer,
			Texture& texture)

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
			VulkanUniformBuffer<T>& uniformBuffer,
			SkyboxTexture& texture)

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