#pragma once
#include"../Scene/Scene.h"


namespace CRATER::UI {
	class Editor {
	private:
		Scene::Scene& m_scene;
		entt::entity m_selected{entt::null};

		// A not-yet-created entity, edited in the "Create Entity" dialog. This is UI
		// state only — it becomes a real ECS entity solely when the user commits it,
		// so the scene never contains a half-built entity.
		struct Draft {
			std::string               name = "Entity";
			Scene::TransformComponent transform{};
			Scene::MeshComponent      mesh{};
			Scene::MaterialComponent  material{};
		};
		Draft m_draft;

		void renderHierarchy();
		void renderInspector();
		void drawCreateEntityPopup();
	 
	public:

		Editor(Scene::Scene& m_scene);
  
		void renderUI();/*The editor gets rendered by function
		callbacks through the renderer,To keep
		layers disconnected*/
		 
	};
}