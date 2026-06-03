#pragma once

 
#include"../constants.h"
#include"../Resources/buffers/uniform_buffer.h"
#include"../../external/spirv_reflect.h"
#include <map>


 

namespace CRATER::Renderer {
	class DescriptorSetLayout {
	private:
		vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
		struct BindingInfo {
			uint32_t binding;
			vk::DescriptorType type;
			vk::ShaderStageFlags stage;
		};

		std::vector<vk::DescriptorPoolSize> poolSizes;
		std::map<std::string, BindingInfo> reflectionMap;
	public:
		void reflectShader(const std::vector<char>& spirvCode);

		auto layout() {
			return *descriptorSetLayout;
		}

		const auto& getPoolSizes() {
			return poolSizes;
		}

		const auto& getReflectionMap() {
			return reflectionMap;
		}



		void create(vk::raii::Device& device);
	};

	 
	class DescriptorSet {
	private:
		DescriptorSetLayout* layoutRef;
		vk::raii::DescriptorPool descriptorPool = nullptr;
		vk::raii::PipelineLayout pipelineLayout = nullptr;
	 
		std::vector<vk::raii::DescriptorSet> descriptorSets;
 
		 
	public:
 
		void updateBuffer(vk::raii::Device& device, uint32_t frameIdx, const std::string& name, vk::DescriptorBufferInfo& bufferInfo);
		void updateImage(vk::raii::Device& device, uint32_t frameIdx, const std::string& name, vk::DescriptorImageInfo& imageInfo);

		 
		void create(vk::raii::Device& device, DescriptorSetLayout* descriptorSetLayout);
	
		auto& operator[] (uint32_t frameIndex) {
			return descriptorSets[frameIndex];
		}
		 
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

