#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include"systems/camera_system.h"
#include"../engineTypes.h"

namespace CRATER::Scene {

	// Simple, data-only components for an entt-based ECS

	struct TagComponent {
		std::string tag;
	};

	struct UUIDComponent {
		uint64_t id = 0;
	};

	struct TransformComponent {
		glm::vec3 position{0.0f};
		glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
		glm::vec3 scale{1.0f};

		mutable glm::mat4 transformMatrix{1.0f};
		mutable bool transformDirty = true;

		void SetPosition(const glm::vec3& pos) { position = pos; transformDirty = true; }
		void SetRotation(const glm::quat& rot) { rotation = rot; transformDirty = true; }
		void SetScale(const glm::vec3& scl) { scale = scl; transformDirty = true; }

		const glm::vec3& GetPosition() const { return position; }
		const glm::quat& GetRotation() const { return rotation; }
		const glm::vec3& GetScale() const { return scale; }

		glm::mat4 GetTransformMatrix() const {
			if (transformDirty) {
				glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
				glm::mat4 rotationMat = glm::mat4_cast(rotation);
				glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);
				transformMatrix = translation * rotationMat * scaleMat;
				transformDirty = false;
			}
			return transformMatrix;
		}

		TransformComponent(
			const glm::vec3& pos,
			const glm::quat& rot,
			const glm::vec3& scl
		) : position(pos), rotation(rot), scale(scl) {
		}
	};

	// Mesh and material are represented by resource identifiers (strings) so the component
	// does not depend on concrete resource types. Rendering should be implemented in a system.
	struct MeshComponent {
		std::string meshID;
		std::string modelPath;
		 
	};
 

	struct MaterialComponent {
		std::string materialID;
		std::string shaderID;
		PipelineType type= PipelineType::OpaqueMesh;
	};

	struct SkyboxComponent {
		std::string skyboxID;
		std::string skyboxPath;
	};

 

}
