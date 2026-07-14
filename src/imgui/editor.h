#pragma once
#include"../Scene/Scene.h"


namespace CRATER::UI {
	class Editor {
	private:
		Scene::Scene& m_scene;
	 
	public:

		Editor(Scene::Scene& m_scene);
  
		void renderUI();/*The editor gets rendered by function
		callbacks through the renderer,To keep
		layers disconnected*/
		 
	};
}