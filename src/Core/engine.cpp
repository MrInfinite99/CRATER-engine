#include "engine.h"

namespace CRATER {
	 
	void Engine::mainLoop(CRATER::Scene::Scene& scene) {
		SDL_Event e;
		bool running = true;


		auto lastTime = std::chrono::high_resolution_clock::now();


		while (running) {
			// Handle input/events
			// Update scene
			// Render scene
			auto currentTime = std::chrono::high_resolution_clock::now();
			float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
			lastTime = currentTime;

			while (SDL_PollEvent(&e)) {
				if (e.type == SDL_EVENT_QUIT) {
					running = false;
				}
				processInput(e,scene,deltaTime);
			}
			renderer->render(scene);
		}
	 
		renderer->wait();
	}

	void Engine::cleanup() {
		// Clean up resources and subsystems
		renderer->cleanUp();
	}

	void Engine::processInput(SDL_Event& e,CRATER::Scene::Scene& scene,float deltaTime) {
		// close the window when user alt-f4s or clicks the X button
		
		if (e.type == SDL_EVENT_WINDOW_RESIZED) {
			renderer->resized();
		}

		scene.processEvents(e, deltaTime);

	}
}