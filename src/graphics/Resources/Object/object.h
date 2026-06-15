#pragma once
#include"material.h"
#include"mesh.h"

namespace CRATER::Resource {
	struct RenderObject {
		glm::vec3 position = { 0.0f,0.0f,0.0f };
		glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
		glm::vec3 scale = { 1.0f, 1.0f, 1.0f };


		struct MeshBinding {
			ResourceHandle<Mesh>    mesh;
			ResourceHandle<Texture> texture;  // correct texture for this primitive
		};
		std::vector<MeshBinding> meshBindings;
		ResourceHandle<Material> material;


		glm::mat4 getModelMatrix() const {
			glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
			model = model * glm::mat4_cast(rotation); // Convert Quat to Mat4 correctly
			model = glm::scale(model, scale);
			return model;
		}
	};


	struct SkyboxData {//this is really annoying
		std::vector<SkyboxVertex> vertices;
		std::vector<uint32_t> indices;
		int materialIndex = -1;
	};

	class Skybox :public Resource{

	public:

		Skybox(const std::string& id,
			const std::string& ktxPath,
			Renderer::VulkanDevice* device,
			Renderer::VulkanSwapChain* swapChain,
			vk::Format format,
			VmaAllocator allocator,
			ResourceManager* resourceManager
		) :Resource(id),m_id(id),
			m_path(ktxPath),
			m_device(device),
			m_swapChain(swapChain),
			m_allocator(allocator),
			m_resourceManager(resourceManager)
		{

			std::vector<SkyboxVertex> skyboxVertices = {
   {{-1.0f, -1.0f, -1.0f}},  // 0
   {{ 1.0f, -1.0f, -1.0f}},  // 1
   {{ 1.0f,  1.0f, -1.0f}},  // 2
   {{-1.0f,  1.0f, -1.0f}},  // 3
   {{-1.0f, -1.0f,  1.0f}},  // 4
   {{ 1.0f, -1.0f,  1.0f}},  // 5
   {{ 1.0f,  1.0f,  1.0f}},  // 6
   {{-1.0f,  1.0f,  1.0f}},  // 7
			};

			std::vector<uint32_t> skyboxIndices = {
				5, 4, 7,  5, 7, 6,  // Front  (+Z)
				0, 1, 2,  0, 2, 3,  // Back   (-Z)
				4, 0, 3,  4, 3, 7,  // Left   (-X)
				1, 5, 6,  1, 6, 2,  // Right  (+X)
				3, 2, 6,  3, 6, 7,  // Top    (+Y)
				4, 5, 1,  4, 1, 0,  // Bottom (-Y)
			};

			skyboxData.vertices = skyboxVertices;
			skyboxData.indices = skyboxIndices;
			skyboxData.materialIndex = -1;

			auto shaderPath = "D:/vkguide/VkRE/shaders/skybox.slang";

			m_skyboxShader=resourceManager->Load<Shader>(id,&m_device->logicalDevice(), m_swapChain, format, PipelineType::Skybox, shaderPath, "vertMain", "fragMain");
 	
		}

		bool doLoad() override {
			m_texture = m_resourceManager->Load<SkyboxTexture>(m_id,m_path,m_allocator,m_device);
			m_material = m_resourceManager->Load<Material>(m_id,m_device,m_skyboxShader);

			m_vertexBuffer.createVertexBuffer(
				skyboxData.vertices,
				m_allocator,
				*m_device
			);
			m_indexBuffer.createIndexBuffer(
				skyboxData.indices,
				m_allocator,
				*m_device
			);
			return true;
		}

		VulkanIndexBuffer& getIndexBuffer(){
			return m_indexBuffer;
		}

		VulkanVertexBuffer& getVertexBuffer() {
			return m_vertexBuffer;
		}

			
		void doUnload() override {
			return;
		}

		Material* getMaterial() const {
			return m_material.Get();
		}

		SkyboxTexture* getTexture() const {
			return m_texture.Get();
		}
 	
	private:
		ResourceHandle<SkyboxTexture> m_texture;
		ResourceHandle<Shader> m_skyboxShader;
		SkyboxData skyboxData;
		std::string m_path;
		Renderer::VulkanDevice* m_device;
		Renderer::VulkanSwapChain* m_swapChain;
		VmaAllocator m_allocator;
		ResourceHandle<Material> m_material;
		VulkanIndexBuffer m_indexBuffer;
		VulkanVertexBuffer m_vertexBuffer;
		ResourceManager* m_resourceManager;
		std::string m_id;
		
 

	};
}