#pragma once
#include "../graphics/Renderer/renderer.h"

namespace CRATER {
	class Engine {
	public:


		Engine() {
			std::cout << "starting" << std::endl;
			renderer = std::make_unique<CRATER::Renderer::Renderer>();
			//renderer->init();
		}
		
		~Engine() {
			std::cout << "engine stopped" << std::endl;
		};

		void run() {
			 
			mainLoop();
			cleanup();
		}
		 
	private:
		 
		void mainLoop();
		void cleanup();

		std::unique_ptr<Renderer::Renderer> renderer{ nullptr };
	};
}