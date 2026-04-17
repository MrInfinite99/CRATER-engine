#pragma once

#include <entt/entt.hpp>
#include <memory>
#include "components.h"

namespace CRATER::Scene {

 
	// Thin entt-backed Entity wrapper
	class Entity {
	public:
		Entity() = default;
		Entity(entt::entity handle, entt::registry* registry) : m_Handle(handle), m_registry(registry) {}

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args) {
			return m_registry->emplace<T>(m_Handle, std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent() {
			return m_registry->get<T>(m_Handle);
		}

		template<typename T>
		bool HasComponent() const {
			return m_registry->all_of<T>(m_Handle);
		}

		template<typename T>
		void RemoveComponent() {
			m_registry->remove<T>(m_Handle);
		}

		entt::entity GetHandle() const { return m_Handle; }

		explicit operator bool() const { return m_Handle != entt::null && m_registry != nullptr; }

	private:
		entt::entity m_Handle{entt::null};
		 
		entt::registry* m_registry{ nullptr };
	};

}
