#pragma once
#include "../graphics/Renderer/vulkanContext.h"
#include "../graphics/Renderer/renderer.h"

namespace CRATER {
	class Engine {
	public:
		Engine() = default;
		~Engine() = default;

		void init(CRATER::Scene::Scene& scene);
		void run(CRATER::Scene::Scene& scene);

	private:
		void mainLoop(CRATER::Scene::Scene& scene);
		 
		void processInput(SDL_Event& e, float deltaTime, CRATER::Scene::Scene& scene);

		std::unique_ptr<Renderer::VulkanContext> m_context;
		std::unique_ptr<Renderer::Renderer>      m_renderer;
	};
}
