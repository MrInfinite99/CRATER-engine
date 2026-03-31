#include "engine.h"

namespace CRATER {
	Engine::Engine() {
		// Initialize subsystems here (e.g. window, renderer, resource manager)
	}
	Engine::~Engine() {
		// Clean up subsystems here
	}
	
	void Engine::init() {
		// Initialize the engine (e.g. create window, initialize renderer)
	}

	void Engine::mainLoop() {
		// Main game loop
		bool running = true;
		while (running) {
			// Handle input/events
			// Update scene
			// Render scene
		}
	}

	void Engine::cleanup() {
		// Clean up resources and subsystems
	}
}