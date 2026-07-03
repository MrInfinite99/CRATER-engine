#pragma once
#include<glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>


struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 texCoord;
	glm::vec4 tangent;
	

	bool operator==(const Vertex& other) const {
		//return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}

};

struct SkyboxVertex {
	glm::vec3 pos;
};



namespace std {
	/*template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};*/
}
