#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include"../Resources/shaders/shader_compile.h"
#include"../Resources/buffers/vertex_buffer.h"
#include"descriptor_set.h"
#include"../../engineTypes.h"

namespace CRATER::Renderer {
	 

	struct PipelineConfig
	{
		std::string vertexEntryPoint = "vertMain";
		std::string fragmentEntryPoint = "fragMain";

		// Vertex Input (Defaults can be overridden or cleared)
		std::vector<vk::VertexInputBindingDescription> bindingDescriptions = Resource::getBindingDescription();
		std::vector<vk::VertexInputAttributeDescription> attributeDescriptions = Resource::getAttributeDescriptions();

		// Fixed-Function States
		vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
		vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
		vk::CullModeFlags cullMode = vk::CullModeFlagBits::eFront;
		vk::FrontFace frontFace = vk::FrontFace::eClockwise;

		// Depth / Stencil
		vk::Bool32 depthTestEnable = vk::True;
		vk::Bool32 depthWriteEnable = vk::True;
		vk::CompareOp depthCompareOp = vk::CompareOp::eLess;

		// Color Blending (Defaults to opaque)
		vk::PipelineColorBlendAttachmentState colorBlendAttachment{
			.blendEnable = vk::False,
			.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
							  vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
		};
	};

	inline void getConfiguredPipeline(PipelineType type, PipelineConfig& config) {
		switch (type) {
		case PipelineType::Skybox:
			config.depthWriteEnable = vk::False;
			config.depthCompareOp = vk::CompareOp::eLessOrEqual;
			config.cullMode = vk::CullModeFlagBits::eNone;
			break;
		case PipelineType::OpaqueMesh:
			break;
		}
	}
	
}