#pragma once
#include<tiny_obj_loader.h>
#include<unordered_map>
#include"../vertex.h"


namespace CRATER::ResourceManager {
	class Model {
	public:
		void load(const char* model_path) {
			tinyobj::attrib_t attrib;
			std::vector<tinyobj::shape_t> shapes;
			std::vector<tinyobj::material_t> materials;
			std::string warn, err;

			std::unordered_map<Vertex, uint32_t> uniqueVertices{};

			if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,model_path)) {
				throw std::runtime_error(warn + err);
			}

			for (const auto& shape : shapes) {
				for (const auto& index : shape.mesh.indices) {
					Vertex vertex{};



					vertex.pos = {
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2]
					};

					if (index.texcoord_index >= 0) {
						vertex.texCoord = {
								attrib.texcoords[2 * index.texcoord_index + 0],
								1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
						};
					}
					vertex.color = { 1.0f, 1.0f, 1.0f };



					if (uniqueVertices.count(vertex) == 0) {
						uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
						vertices.push_back(vertex);
					}
					indices.push_back(uniqueVertices[vertex]);
				}
			}
		}

			const std::vector<Vertex>& getVertices() {
				return vertices;
			}

			const std::vector<uint32_t>& getIndices() {
				return indices;
			}

	private:
		
		
		std::vector<uint32_t> indices;

		std::vector<Vertex> vertices;
	};
};