#pragma once

#include <entt/entt.hpp>
#include <vector>
#include <memory>
#include "Entity.h"
#include "System.h"
 

namespace CRATER::Scene {

	class Scene
	{
	public:
		Scene() = default;
		~Scene() = default;

		entt::registry& getRegistry() { return m_Registry; }

		Entity CreateEntity() {
			entt::entity e = m_Registry.create();
			return Entity(e, this);
		}

		void DestroyEntity(Entity entity) {
			if (entity) m_Registry.destroy(entity.GetHandle());
		}

		void Update(float deltaTime) {
			for (auto& system : m_Systems)
				system->update(deltaTime, *this);
		}

		void Render() {
			for (auto& system : m_Systems)
				; // Rendering systems will be executed via Update or separate ordering
		}

		void Initialize() {
			for (auto& system : m_Systems)
				system->init();
		}

		void AddSystem(std::unique_ptr<System> system) {
			m_Systems.push_back(std::move(system));
		}

		// helper to add a RenderSystem backed by the provided renderer & resource manager
		//void AddRenderSystem(CRATER::Renderer::Renderer* renderer, CRATER::Renderer::ResourceManager* resources) {
		//	m_Systems.push_back(std::make_unique<CRATER::Renderer::RenderSystem>(renderer, resources));
		//}

		// expose registry for systems that need it
		entt::registry& Registry() { return m_Registry; }

	private:
		entt::registry m_Registry;
		std::vector<std::unique_ptr<System>> m_Systems;
	};

}
