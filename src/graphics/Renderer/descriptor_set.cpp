#include"descriptor_set.h"

namespace CRATER::Renderer {




	void DescriptorSet::reflectShader(const std::vector<char>& spirvCode) {
		SpvReflectShaderModule module;
		spvReflectCreateShaderModule(spirvCode.size(), spirvCode.data(), &module);

		uint32_t count = 0;
		spvReflectEnumerateDescriptorSets(&module, &count, nullptr);
		std::vector<SpvReflectDescriptorSet*> sets(count);
		spvReflectEnumerateDescriptorSets(&module, &count, sets.data());

		// We assume set 0 for this specific class instance
		if (count > 0) {
			auto* set = sets[0];
			for (uint32_t i = 0; i < set->binding_count; ++i) {
				auto* b = set->bindings[i];

				// Check if we already found this binding in a previous stage
				if (reflectionMap.find(b->name) == reflectionMap.end()) {
					reflectionMap[b->name] = {
						b->binding,
						static_cast<vk::DescriptorType>(b->descriptor_type),
						static_cast<vk::ShaderStageFlags>(module.shader_stage) // Start with current stage
					};
				}
				else {
					// If it exists, just add the new stage flag (e.g., Vertex | Fragment)
					reflectionMap[b->name].stage |= static_cast<vk::ShaderStageFlags>(module.shader_stage);
				}

				// Track pool sizes needed
				poolSizes.push_back({ static_cast<vk::DescriptorType>(b->descriptor_type), MAX_FRAMES_IN_FLIGHT });
			}
		}
		spvReflectDestroyShaderModule(&module);
	}


	void DescriptorSet::build(vk::raii::Device& device) {
		std::vector<vk::DescriptorSetLayoutBinding> layoutBindings;

		for (auto const& [name, info] : reflectionMap) {
			layoutBindings.emplace_back(info.binding, info.type, 1, vk::ShaderStageFlagBits::eAll);
		}

		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
		layoutInfo.pBindings = layoutBindings.data();
		descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);

		vk::DescriptorPoolCreateInfo poolInfo{
			.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
			.maxSets = MAX_FRAMES_IN_FLIGHT,
			.poolSizeCount = (uint32_t)poolSizes.size(),
			.pPoolSizes = poolSizes.data()
		};
		descriptorPool = vk::raii::DescriptorPool(device, poolInfo);

		// Allocate sets
		std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *descriptorSetLayout);
		vk::DescriptorSetAllocateInfo allocInfo{};
		allocInfo.descriptorPool = *descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
		allocInfo.pSetLayouts = layouts.data();
		descriptorSets = device.allocateDescriptorSets(allocInfo);
	}


	void DescriptorSet::updateBuffer(vk::raii::Device& device, uint32_t frameIdx, const std::string& name, vk::DescriptorBufferInfo bufferInfo) {
		auto& info = reflectionMap.at(name);

		vk::WriteDescriptorSet write{
			.dstSet = *descriptorSets[frameIdx],
			.dstBinding = info.binding,
			.descriptorCount = 1,
			.descriptorType = info.type,
			.pBufferInfo = &bufferInfo
		};
		device.updateDescriptorSets(write, nullptr);
	}



	void DescriptorSet::updateImage(vk::raii::Device& device, uint32_t frameIdx, const std::string& name, vk::DescriptorImageInfo imageInfo) {
		auto& info = reflectionMap.at(name);

		vk::WriteDescriptorSet write{
			.dstSet = *descriptorSets[frameIdx],
			.dstBinding = info.binding,
			.descriptorCount = 1,
			.descriptorType = info.type,
			.pImageInfo = &imageInfo
		};
		device.updateDescriptorSets(write, nullptr);
	}





}