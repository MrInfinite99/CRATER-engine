#include "graphics_pipeline.h"

namespace CRATER::Renderer {
	

	void PipelineLayout::create(vk::raii::Device& device,DescriptorSetLayout& descriptorSetLayout, PushConstant& pushConstant) {
 
        auto layout = descriptorSetLayout.layout();

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
            .setLayoutCount = 1,
            .pSetLayouts = &layout
        };

        // Check if the shader actually uses push constants.
        // Replace '.size' or '.stageFlags' with whatever getters your PushConstant struct uses.
        auto range = pushConstant.getRange();

        if (range.size > 0 && range.stageFlags != vk::ShaderStageFlags(0))
        {
            pipelineLayoutInfo.pushConstantRangeCount = 1;
            pipelineLayoutInfo.pPushConstantRanges = &pushConstant.getRange();
        }
        else
        {
            pipelineLayoutInfo.pushConstantRangeCount = 0;
            pipelineLayoutInfo.pPushConstantRanges = nullptr;
        }

		m_pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);
	}
}



namespace CRATER::Renderer {
	vk::raii::ShaderModule GraphicsPipeline::createShaderModule(const std::vector<char>& code, vk::raii::Device& device) const {
		vk::ShaderModuleCreateInfo createInfo{
			.codeSize = code.size() * sizeof(char),
			.pCode = reinterpret_cast<const uint32_t*>(code.data()) };

		vk::raii::ShaderModule shaderModule{ device, createInfo };

		return shaderModule;
	}
	 
	void GraphicsPipeline::create(PipelineLayout& layout,vk::raii::Device& device, vk::Extent2D& swapChainExtent, vk::Format& swapChainImageFormat, vk::SurfaceFormatKHR   swapChainSurfaceFormat, const std::vector<char>& code,vk::Format depthFormat,PipelineType type) {
		m_shaderModule = createShaderModule(code, device);

        getConfiguredPipeline(type, config);

		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
		.stage = vk::ShaderStageFlagBits::eVertex,
		.module = m_shaderModule,
		.pName = config.vertexEntryPoint.c_str()
		};
		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
			.stage = vk::ShaderStageFlagBits::eFragment,
			.module = m_shaderModule,
			.pName = config.fragmentEntryPoint.c_str()
		};


		vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
		
		std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

		vk::PipelineDynamicStateCreateInfo dynamicState{ .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data() };

		 
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
           .vertexBindingDescriptionCount = static_cast<uint32_t>(config.bindingDescriptions.size()),
           .pVertexBindingDescriptions = config.bindingDescriptions.data(),
           .vertexAttributeDescriptionCount = static_cast<uint32_t>(config.attributeDescriptions.size()),
           .pVertexAttributeDescriptions = config.attributeDescriptions.data()
        };

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ .topology = config.topology };

        vk::PipelineViewportStateCreateInfo viewportState{ .viewportCount = 1, .scissorCount = 1 };

        // Generic Rasterization State
        vk::PipelineRasterizationStateCreateInfo rasterizer{
            .depthClampEnable = vk::False,
            .rasterizerDiscardEnable = vk::False,
            .polygonMode = config.polygonMode,
            .cullMode = config.cullMode,
            .frontFace = config.frontFace,
            .depthBiasEnable = vk::False,
            .lineWidth = 1.0f
        };

        vk::PipelineMultisampleStateCreateInfo multisampling{
            .rasterizationSamples = vk::SampleCountFlagBits::e1,
            .sampleShadingEnable = vk::False
        };

        // Generic Depth/Stencil State
        vk::PipelineDepthStencilStateCreateInfo depthStencil{
            .depthTestEnable = config.depthTestEnable,
            .depthWriteEnable = config.depthWriteEnable,
            .depthCompareOp = config.depthCompareOp,
            .depthBoundsTestEnable = vk::False,
            .stencilTestEnable = vk::False
        };

        // Generic Color Blending State
        vk::PipelineColorBlendStateCreateInfo colorBlending{
            .logicOpEnable = vk::False,
            .logicOp = vk::LogicOp::eCopy,
            .attachmentCount = 1,
            .pAttachments = &config.colorBlendAttachment
        };

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
        pipelineInfo.layout = layout.layout();
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