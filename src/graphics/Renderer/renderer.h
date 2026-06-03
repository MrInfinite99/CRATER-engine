#pragma once
#include"../../Core/window.h"
#include "validationLayers.h"
#include"../Resources/vma/vma_allocator.h"
#include"../Resources/textures/texture.h"
#include"../Resources/models/model.h"
#include"../constants.h"
#include"../../Scene/Scene.h"
#include"../Object/material.h"
#include"../Object/mesh.h"
 

namespace CRATER::Renderer {


	struct RenderObject {
		glm::vec3 position = { 0.0f,0.0f,0.0f };
		glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
		glm::vec3 scale = { 1.0f, 1.0f, 1.0f };


		Object::Mesh* mesh;
		Object::Material* material;

		glm::mat4 getModelMatrix() const {
			glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
			model = model * glm::mat4_cast(rotation); // Convert Quat to Mat4 correctly
			model = glm::scale(model, scale);
			return model;
		}
	};
}


 

namespace CRATER::Renderer
{
	class Renderer {
	public:
		Renderer() {
			createInstance();
			setupDebugMessenger();
			createSurface();
			// create logical device
			m_device.init(m_context, m_instance, m_surface);

			initAllocator();

			m_swapChain.createSwapChain(m_device.logicalDevice(),m_device.physicalDevice(), m_surface, m_window);
			m_swapChain.createImageViews(m_device.logicalDevice(),1);
			
			createCommandPool();
			depthtexture.createDepthResources(m_device, m_swapChain.extent());
			uniformBuffer.createUniformBuffer(m_allocator, m_device);

			createCommandBuffer();

			createSyncObjects();
		};

		void setup(CRATER::Scene::Scene& scene) {
			 
			auto view = scene.getRegistry().view<
				CRATER::Scene::TransformComponent,
				CRATER::Scene::MeshComponent,
				CRATER::Scene::MaterialComponent
				> ();


			view.each([&](
				CRATER::Scene::TransformComponent& transform,
				CRATER::Scene::MeshComponent& meshComp,
				CRATER::Scene::MaterialComponent& materialComp) {

					Object::Material* material = createMaterial(materialComp.materialID,materialComp.materialPath,materialComp.texturePath,materialComp.type);
					Object::Mesh* mesh = createMesh(meshComp.meshID,meshComp.modelPath);

					RenderObject renderObj;
					renderObj.mesh = mesh;
					renderObj.material = material;
					renderObj.position = transform.position;
					renderObj.rotation = transform.rotation;
					renderObj.scale = transform.scale;
 
					renderObjects.push_back(std::move(renderObj));
				}
			);
		
			//update descriptor set
			for (auto& [name, material] : materials) {
				for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
					materials[name]->updateDescriptors(m_device.logicalDevice(), i, uniformBuffer,*textures[name]);
				}
			}

			for (auto& [name, material] : skyboxMaterials) {
				for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
					skyboxMaterials[name]->updateDescriptors(m_device.logicalDevice(), i, uniformBuffer, *skyboxTextures[name]);
				}
			}

		  

		}

		 

		void render(CRATER::Scene::Scene& scene);

		void wait() {
			m_device.logicalDevice().waitIdle();
		}

		void cleanUp() {
			m_window.cleanUp();
 
		}

		void resized() {
			framebufferResized = true;
		}

		~Renderer() { 
		}
		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
	private:
		void createInstance();
		void setupDebugMessenger();
		void createSurface();
		void createCommandPool();
		void createCommandBuffer();
		void recordCommandBuffer(uint32_t imageIndex);
		void createSyncObjects();
		void initAllocator();
		
		Object::Material* createMaterial(const std::string& matID,const std::string& matPath,const std::string& texPath, PipelineType type);
		Object::Mesh* createMesh(const std::string& meshID, const std::string& meshPath);
		Object::Shader* createShader(const std::string& shaderPath, PipelineType type);

		void transition_image_layout(
			vk::Image image,
			vk::ImageLayout oldLayout,
			vk::ImageLayout newLayout,
			vk::AccessFlags2 srcAccessMask,
			vk::AccessFlags2 dstAccessMask,
			vk::PipelineStageFlags2 srcStageMask,
			vk::PipelineStageFlags2 dstStageMask,
			vk::ImageAspectFlags    image_aspect_flags
		);

		ValidationLayers validationLayers{ true };
		vk::raii::Context m_context;
		vk::raii::Instance m_instance{ nullptr };
		vk::raii::DebugUtilsMessengerEXT debugMessenger{ nullptr };

		
		VulkanDevice m_device{};
		Window m_window{ "CRATER", WIDTH, HEIGHT };
		vk::raii::SurfaceKHR m_surface{ nullptr };
		VulkanSwapChain m_swapChain{};
	 
		vk::raii::CommandPool m_commandPool{ nullptr };

		VmaAllocatorRAII m_allocator;

		std::vector<vk::raii::CommandBuffer> commandBuffers;

		std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
		std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
		std::vector<vk::raii::Fence> inFlightFences;
		//std::optional<vk::raii::Queue> graphicsQueue;
		 
		std::optional<vk::raii::Queue> presentQueue;

		uint32_t frameIndex = 0;

		bool framebufferResized = false;

		std::vector<char> shaderCode;
		Object::Shader shader{};
		 

		Resource::VulkanUniformBuffer<UniformBufferObject> uniformBuffer{};
	  
		 
		Resource::Texture depthtexture{};
		PushConstant pushConstant{};
		std::vector<RenderObject> renderObjects;
		std::unordered_map<std::string, std::unique_ptr<Object::Shader>> shaders;
		std::unordered_map<std::string, std::unique_ptr<Object::Mesh>> meshes;
		std::unordered_map<std::string, std::unique_ptr<Object::Material>> materials;
		std::unordered_map<std::string, std::unique_ptr<Object::Material>> skyboxMaterials;
		std::unordered_map<std::string, std::unique_ptr<Resource::Texture>> textures;
		std::unordered_map<std::string, std::unique_ptr<Resource::SkyboxTexture>> skyboxTextures;


	
	};
}