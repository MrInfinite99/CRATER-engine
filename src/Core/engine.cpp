#include "engine.h"
#include <imgui_impl_sdl3.h>

namespace CRATER {

	void Engine::init(CRATER::Scene::Scene& scene) {
		static LogStreambuf errCapture{ Log::Level::Error, std::cerr.rdbuf() };
		static LogStreambuf outCapture{ Log::Level::Info,  std::cout.rdbuf() };
		std::cerr.rdbuf(&errCapture);
		std::cout.rdbuf(&outCapture);

		m_context = std::make_unique<Renderer::VulkanContext>();
		m_context->init();

		m_renderer = std::make_unique<Renderer::Renderer>(*m_context);
		m_editor = std::make_unique<UI::Editor>(scene);
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
			const bool* keys = SDL_GetKeyboardState(nullptr);
			while (SDL_PollEvent(&e)) {
				if (e.type == SDL_EVENT_QUIT)
					running = false;
				if (keys[SDL_SCANCODE_ESCAPE]) {
					running = false;
				}
				processInput(e, deltaTime, scene);
			}
		 
			m_renderer->sync(scene);
			m_renderer->render(scene,
				[this] {return m_editor->renderUI(); }
				 );
			
		}

		m_renderer->wait();
	}

	 

	void Engine::processInput(SDL_Event& e, float deltaTime, CRATER::Scene::Scene& scene) {
		if (e.type == SDL_EVENT_WINDOW_RESIZED)
			m_renderer->resized();

		ImGui_ImplSDL3_ProcessEvent(&e);

		// Don't drive the camera while ImGui is using the mouse/keyboard.
		if (!(ImGui::GetIO().WantCaptureMouse|| ImGui::GetIO().WantCaptureKeyboard))
			scene.processEvents(e, deltaTime);
	}
}
