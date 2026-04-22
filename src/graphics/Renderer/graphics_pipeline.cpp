#include "graphics_pipeline.h"

namespace CRATER::Renderer {

	 

	vk::raii::ShaderModule VulkanGraphicsPipeline::createShaderModule(const std::vector<char>& code, vk::raii::Device& device) const{
		vk::ShaderModuleCreateInfo createInfo{
			.codeSize = code.size() * sizeof(char),
			.pCode = reinterpret_cast<const uint32_t*>(code.data()) };

		vk::raii::ShaderModule shaderModule{ device, createInfo };

		return shaderModule;
	}


	void VulkanGraphicsPipeline::createPipeline(vk::raii::Device& device, vk::Extent2D& swapChainExtent, vk::Format& swapChainImageFormat, vk::SurfaceFormatKHR   swapChainSurfaceFormat, const std::vector<char>& code, DescriptorSet& descriptorSets,PushConstant& pushConstant, vk::Format depthFormat) {
		m_shaderModule = createShaderModule(code, device);

		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eVertex, .module = m_shaderModule,  .pName = "vertMain" };
		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eFragment, .module = m_shaderModule, .pName = "fragMain" };
		vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

		vk::PipelineDynamicStateCreateInfo dynamicState{ .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data() };

		auto bindingDescription = ResourceManager::getBindingDescription();
		auto attributeDescriptions = ResourceManager::getAttributeDescriptions();
		vk::PipelineVertexInputStateCreateInfo   vertexInputInfo{ .vertexBindingDescriptionCount = 1,
														 .pVertexBindingDescriptions = &bindingDescription,
														 .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
														 .pVertexAttributeDescriptions = attributeDescriptions.data() };

		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ .topology = vk::PrimitiveTopology::eTriangleList };

		vk::Viewport viewport{ 0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f };

		vk::PipelineViewportStateCreateInfo viewportState{ .viewportCount = 1, .scissorCount = 1 };

		vk::PipelineRasterizationStateCreateInfo rasterizer{ .depthClampEnable = vk::False,
													.rasterizerDiscardEnable = vk::False,
													.polygonMode = vk::PolygonMode::eFill,
													.cullMode = vk::CullModeFlagBits::eNone,
													.frontFace = vk::FrontFace::eClockwise,
													.depthBiasEnable = vk::False,
													.lineWidth = 1.0f };

		vk::PipelineMultisampleStateCreateInfo multisampling{ .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False };

		vk::PipelineDepthStencilStateCreateInfo depthStencil{
	.depthTestEnable = vk::True,
	.depthWriteEnable = vk::True,
	.depthCompareOp = vk::CompareOp::eLess,
	.depthBoundsTestEnable = vk::False,
	.stencilTestEnable = vk::False };


		vk::PipelineColorBlendAttachmentState colorBlendAttachment{
	.blendEnable = vk::False,
	.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };

		vk::PipelineColorBlendStateCreateInfo colorBlending{
	.logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments = &colorBlendAttachment };

		auto layout = descriptorSets.layout();

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
			.setLayoutCount = 1,
			.pSetLayouts = &layout,
			.pushConstantRangeCount = 1,
			.pPushConstantRanges=&pushConstant.getRange()
		};

		m_pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);
 
		vk::GraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = m_pipelineLayout;
		pipelineInfo.renderPass = nullptr;

		vk::PipelineRenderingCreateInfo renderingInfo{};
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachmentFormats = &swapChainSurfaceFormat.format;
		renderingInfo.depthAttachmentFormat = depthFormat;

		vk::StructureChain<
			vk::GraphicsPipelineCreateInfo,
			vk::PipelineRenderingCreateInfo
		> pipelineCreateInfoChain(pipelineInfo, renderingInfo);

		m_graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
	}
	
}