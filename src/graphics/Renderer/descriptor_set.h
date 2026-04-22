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

	class PushConstant {
	public:
		void reflectShader(const std::vector<char>& spirvCode);

		const vk::PushConstantRange& getRange() const { return m_range; }
 
		uint32_t getSize() const { return m_size; }

		// Get offset
		uint32_t getOffset() const { return m_offset; }
 
		bool exists() const { return m_size > 0; }

		// Debug info
		std::string getName() const { return m_name; }
	private:
		 
		vk::PushConstantRange m_range{};
	 
		uint32_t m_size = 0;
		uint32_t m_offset = 0;
		std::string m_name;
	};
};

