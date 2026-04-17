#pragma once

 
#include"../constants.h"
#include"../Resources/uniform_buffer.h"
#include"../../external/spirv_reflect.h"
#include <map>


 

namespace CRATER::Renderer {

	 
	class DescriptorSet {
	private:
		 
		vk::raii::DescriptorPool descriptorPool = nullptr;
		vk::raii::PipelineLayout pipelineLayout = nullptr;
		vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
		std::vector<vk::raii::DescriptorSet> descriptorSets;

		//bindings
		struct BindingInfo {
			uint32_t binding;
			vk::DescriptorType type;
			vk::ShaderStageFlags stage;
		 };

		std::map<std::string, BindingInfo> reflectionMap;

		// For internal pool creation
		std::vector<vk::DescriptorPoolSize> poolSizes;


		//void createDescriptorPool(vk::raii::Device& device);
		 
	public:
		void reflectShader(const std::vector<char>& spirvCode);
		void build(vk::raii::Device& device);
		void updateBuffer(vk::raii::Device& device, uint32_t frameIdx, const std::string& name, vk::DescriptorBufferInfo bufferInfo);
		void updateImage(vk::raii::Device& device, uint32_t frameIdx, const std::string& name, vk::DescriptorImageInfo imageInfo);

		//void addBinding(uint32_t binding, vk::DescriptorType type, uint32_t count, vk::ShaderStageFlags stage);

		//void createDescriptorSets(vk::raii::Device& device,ResourceManager::VulkanUniformBuffer<T>& uniformBuffer);
	
		auto& operator[] (uint32_t frameIndex) {
			return descriptorSets[frameIndex];
		}
		
		auto layout() {
			return *descriptorSetLayout;
		}

		//void createDescriptorSetLayout(vk::raii::Device& device);

	};
};

