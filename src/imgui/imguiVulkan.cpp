#include "imguiVulkan.h"
#include "../graphics/Renderer/vulkanContext.h"
#include "../graphics/Renderer/swap_chain.h"

#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <array>

namespace CRATER::UI {

	ImGuiVulkan::~ImGuiVulkan() {
		if (!m_initialized) return;
		// Caller must have made the device idle before destroying us.
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGuiVulkan::init(Renderer::VulkanContext& ctx,
		Renderer::VulkanSwapChain& swapChain)
	{
		const uint32_t imageCount = static_cast<uint32_t>(swapChain.getSwapChainImages().size());
		const uint32_t minImageCount = static_cast<uint32_t>(swapChain.getSwapChainImages().size());

		// ── Descriptor pool owned by ImGui (font atlas + any user textures) ──
		std::array<vk::DescriptorPoolSize, 1> poolSizes{
			vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, 64 }
		};
		vk::DescriptorPoolCreateInfo poolInfo{
			.flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
			.maxSets       = 64,
			.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
			.pPoolSizes    = poolSizes.data()
		};
		m_descriptorPool = vk::raii::DescriptorPool(ctx.device.logicalDevice(), poolInfo);

		// ── Core context ──
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		ImGui::StyleColorsDark();

		// ── Platform backend (SDL3) ──
		ImGui_ImplSDL3_InitForVulkan(ctx.window.getSDLWindow());

		// ── Renderer backend (Vulkan, dynamic rendering) ──
		 

		ImGui_ImplVulkan_InitInfo initInfo{};
		initInfo.Instance       = static_cast<VkInstance>(*ctx.instance);
		initInfo.PhysicalDevice = static_cast<VkPhysicalDevice>(*ctx.device.physicalDevice());
		initInfo.Device         = static_cast<VkDevice>(*ctx.device.logicalDevice());
		initInfo.QueueFamily    = ctx.device.graphicsIndex();
		initInfo.Queue          = static_cast<VkQueue>(*ctx.device.queue().value());
		initInfo.DescriptorPool = static_cast<VkDescriptorPool>(*m_descriptorPool);
		initInfo.MinImageCount  = minImageCount;
		initInfo.ImageCount     = imageCount;
		initInfo.MSAASamples    = VK_SAMPLE_COUNT_1_BIT;

		// No VkRenderPass — feed the dynamic-rendering pipeline the color format.
		initInfo.UseDynamicRendering = true;
		initInfo.PipelineRenderingCreateInfo = {};
		initInfo.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		initInfo.PipelineRenderingCreateInfo.colorAttachmentCount    = 1;
		auto swapChainImageFormat = static_cast<VkFormat>(swapChain.format());
		initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats =&swapChainImageFormat;

		ImGui_ImplVulkan_Init(&initInfo);
		ImGui_ImplVulkan_CreateFontsTexture();


		m_initialized = true;
	}

	void ImGuiVulkan::beginFrame() {
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
	}

	void ImGuiVulkan::endFrame() {
		ImGui::Render();
	}
 
	void ImGuiVulkan::draw(vk::raii::CommandBuffer& commandBuffer) {
		if (ImDrawData* drawData = ImGui::GetDrawData())
			ImGui_ImplVulkan_RenderDrawData(drawData, static_cast<VkCommandBuffer>(*commandBuffer));
	}
 
}
