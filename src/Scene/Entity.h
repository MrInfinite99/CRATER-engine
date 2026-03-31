#pragma once

#include <entt/entt.hpp>
#include <memory>
#include "components.h"

namespace CRATER::Scene {

	class Scene;

	// Thin entt-backed Entity wrapper
	class Entity {
	public:
		Entity() = default;
		Entity(entt::entity handle, Scene* scene) : m_Handle(handle), m_Scene(scene) {}

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args) {
			return m_Scene->getRegistry().emplace<T>(m_Handle, std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent() {
			return m_Scene->getRegistry().get<T>(m_Handle);
		}

		template<typename T>
		bool HasComponent() const {
			return m_Scene->getRegistry().all_of<T>(m_Handle);
		}

		template<typename T>
		void RemoveComponent() {
			m_Scene->getRegistry().remove<T>(m_Handle);
		}

		entt::entity GetHandle() const { return m_Handle; }

		explicit operator bool() const { return m_Handle != entt::null && m_Scene != nullptr; }

	private:
		entt::entity m_Handle{entt::null};
		Scene* m_Scene{nullptr};
	};

}
