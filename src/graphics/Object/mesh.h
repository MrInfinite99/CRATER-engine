#pragma once
#include"../Resources/buffers/vertex_buffer.h"
#include"../Resources/buffers/index_buffer.h"

namespace CRATER::Object {
	struct Mesh {
		Resource::VulkanVertexBuffer vertexBuffer;
		Resource::VulkanIndexBuffer indexBuffer;
	};

}
