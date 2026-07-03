#pragma once
 
#include"../shaders/shader.h"
#include"../textures/texture.h"
 
 


namespace CRATER::Resource {
	enum class AlphaMode { Opaque, Mask, Blend };

	struct MaterialData {
		int albedoIndex = -1;
		int metallicRoughnessIndex = -1;
		int normalIndex = -1;
		int occlusionIndex = -1;
		int emissiveIndex = -1;

		// scalar fallbacks
		glm::vec4 baseColorFactor = { 1, 1, 1, 1 };
		float     metallicFactor = 1.0f;
		float     roughnessFactor = 1.0f;
		float     normalScale = 1.0f;
		float     occlusionStrength = 1.0f;
		glm::vec3 emissiveFactor = { 0, 0, 0 };

		// misc
		AlphaMode alphaMode = AlphaMode::Opaque;
		float     alphaCutoff = 0.5f;
		bool      doubleSided = false;

	};

	 

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
			Texture& albedo,
			Texture& metallicRoughness,
			Texture& normal,
			Texture& occlusion,
			Texture& emissive)

		{
			vk::DescriptorBufferInfo bInfo{
				uniformBuffer[frameIndex].buffer(),
				0,
				sizeof(UniformBufferObject)
			};
			descriptorSets.updateBuffer(device, frameIndex, "ubo", bInfo);
 

			vk::DescriptorImageInfo baseInfo{ albedo.sampler(), albedo.view(), vk::ImageLayout::eShaderReadOnlyOptimal };
			vk::DescriptorImageInfo metallicInfo{ metallicRoughness.sampler(), metallicRoughness.view(), vk::ImageLayout::eShaderReadOnlyOptimal };
			vk::DescriptorImageInfo normalInfo{ normal.sampler(), normal.view(), vk::ImageLayout::eShaderReadOnlyOptimal };
			vk::DescriptorImageInfo occlusionInfo{ occlusion.sampler(), occlusion.view(), vk::ImageLayout::eShaderReadOnlyOptimal };
			vk::DescriptorImageInfo emissiveInfo{ emissive.sampler(), emissive.view(), vk::ImageLayout::eShaderReadOnlyOptimal };

			descriptorSets.updateImage(
				device,
				frameIndex,
				"baseColorMap",
				 baseInfo);



			descriptorSets.updateImage(
				device,
				frameIndex,
				"metallicRoughnessMap",
				 metallicInfo);

			descriptorSets.updateImage(
				device,
				frameIndex,
				"normalMap",
				 normalInfo);

			descriptorSets.updateImage(
				device,
				frameIndex,
				"occlusionMap",
				 occlusionInfo);

			descriptorSets.updateImage(
				device,
				frameIndex,
				"emissiveMap",
				 emissiveInfo);
			 
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