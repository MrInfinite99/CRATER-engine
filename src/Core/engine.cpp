#include "engine.h"

namespace CRATER {

	void Engine::init(CRATER::Scene::Scene& scene) {
		m_context = std::make_unique<Renderer::VulkanContext>();
		m_context->init();

		m_renderer = std::make_unique<Renderer::Renderer>(*m_context);
		m_renderer->setup(scene);
	}

	void Engine::run(CRATER::Scene::Scene& scene) {
		mainLoop(scene);
	}

	void Engine::mainLoop(CRATER::Scene::Scene& scene) {
		SDL_Event e;
		bool running = true;

		auto lastTime = std::chrono::high_resolution_clock::now();

		while (running) {
			auto currentTime = std::chrono::high_resolution_clock::now();
			float deltaTime  = std::chrono::duration<float>(currentTime - lastTime).count();
			lastTime = currentTime;

			while (SDL_PollEvent(&e)) {
				if (e.type == SDL_EVENT_QUIT)
					running = false;
				processInput(e, deltaTime, scene);
			}
		 
			m_renderer->sync(scene);
			m_renderer->render(scene);
			
		}

		m_renderer->wait();
	}

	 

	void Engine::processInput(SDL_Event& e, float deltaTime, CRATER::Scene::Scene& scene) {
		if (e.type == SDL_EVENT_WINDOW_RESIZED)
			m_renderer->resized();
		scene.processEvents(e, deltaTime);
	}
}
