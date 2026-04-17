#pragma once
#include <SDL3/SDL.h>
#include<SDL3/SDL_vulkan.h>
#include <stdexcept>


namespace CRATER {
	class Window {
	public:
		Window(const char* title, int width, int height);
		Window()=default;
		~Window();
		SDL_Window* getSDLWindow() const { return m_window; }
		int getWidth() const { return m_width; }
		int getHeight() const { return m_height; }

		void init(const char* title, int width, int height);

		void cleanUp() {
			if (m_window) {
				SDL_DestroyWindow(m_window);
			}
			SDL_Quit();
		}
	private:
		SDL_Window* m_window{nullptr};
		int m_width{0};
		int m_height{0};
	};
}