#pragma once
#include "../graphics/Renderer/renderer.h"
 

namespace CRATER {
	class Engine {
	public:


		Engine() {
			std::cout << "starting" << std::endl;
			
			//renderer->init();
		}
		
		~Engine() {
			std::cout << "engine stopped" << std::endl;
		};

		void run(CRATER::Scene::Scene& scene) {
			 
			mainLoop(scene);
			cleanup();
		}

		void init(CRATER::Scene::Scene& scene) {
			renderer = std::make_unique<CRATER::Renderer::Renderer>();
			renderer->setup(scene);
		}
		 
	private:
		float deltaTime{ 0.0f };
		void mainLoop(CRATER::Scene::Scene& scene);
		void cleanup();
		void processInput(SDL_Event& e, CRATER::Scene::Scene& scene, float deltaTime);

		std::unique_ptr<Renderer::Renderer> renderer{ nullptr };
	};
}