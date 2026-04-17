#include "engine.h"

namespace CRATER {
	 
	void Engine::mainLoop() {
		SDL_Event e;
		// Main game loop
		bool running = true;
		while (running) {
			// Handle input/events
			// Update scene
			// Render scene
			while (SDL_PollEvent(&e)) {
				// close the window when user alt-f4s or clicks the X button
				if (e.type == SDL_EVENT_QUIT) {
					running = false;
				}
				else if (e.type == SDL_EVENT_WINDOW_RESIZED) {
					renderer->resized();
				}


			}
			renderer->render();
		}
	 
		renderer->wait();
	}

	void Engine::cleanup() {
		// Clean up resources and subsystems
		renderer->cleanUp();
	}
}