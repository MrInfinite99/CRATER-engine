#pragma once
#include"../buffers/vertex_buffer.h"
#include"../buffers/index_buffer.h"
#include"../resource.h"

namespace CRATER::Resource {

	struct MeshData {
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		int materialIndex = -1;
	};


	class Mesh :public Resource{
	public:
		Mesh(const std::string& id,
			const MeshData& meshData,
			Renderer::VulkanDevice* device,
			VmaAllocator allocator)
			:Resource(id),
			m_md(meshData),
			m_device(device),
			m_allocator(allocator){}

		bool doLoad() override {
			vertexBuffer.createVertexBuffer(
				m_md.vertices,
				m_allocator,
				*m_device
			);
			indexBuffer.createIndexBuffer(
				m_md.indices,
				m_allocator,
				*m_device
			);
			m_md.vertices.clear();
			m_md.indices.clear();

			return true;
		}

		void doUnload() override {
			//do nothing i guess
			return;
		}

		VulkanVertexBuffer& getVertexBuffer(){
			return vertexBuffer;
		}

		VulkanIndexBuffer& getIndexBuffer() {
			return indexBuffer;
		}

		int getMaterialIndex() const { return m_md.materialIndex; }

		 
	private:
		Renderer::VulkanDevice* m_device;
			VmaAllocator m_allocator;
		MeshData m_md;
		VulkanVertexBuffer vertexBuffer;
		VulkanIndexBuffer indexBuffer;
	};
 
}
