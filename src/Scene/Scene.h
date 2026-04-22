#pragma once

#include <entt/entt.hpp>
#include <vector>
#include <memory>
#include "Entity.h"
#include "systems/camera_system.h"
 

namespace CRATER::Scene {

	class Scene
	{
	public:
		Scene() = default;
		~Scene() = default;

		entt::registry& getRegistry() { return m_Registry; }

		Entity CreateEntity() {
			entt::entity e = m_Registry.create();
			return Entity(e, &this->m_Registry);
		}

		void DestroyEntity(Entity entity) {
			if (entity) m_Registry.destroy(entity.GetHandle());
		}

		void Update(float deltaTime) {
			 
		}

		void processEvents(SDL_Event& event,float deltaTime) {
			cameraSystem.processInput(event, deltaTime);
		}

		 
		auto& getCamera() {
			return cameraSystem;
		}

		void initialize(glm::vec3 position, glm::vec3 up,float yaw,float pitch) {

			cameraSystem.init(position, up, yaw, pitch);
		}

		// helper to add a RenderSystem backed by the provided renderer & resource manager
		//void AddRenderSystem(CRATER::Renderer::Renderer* renderer, CRATER::Renderer::ResourceManager* resources) {
		//	m_Systems.push_back(std::make_unique<CRATER::Renderer::RenderSystem>(renderer, resources));
		//}

		 

	private:
		entt::registry m_Registry;
		CameraSystem cameraSystem;
	};

}
