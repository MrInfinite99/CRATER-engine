#include "window.h"

namespace CRATER {
	Window::Window(const char* title, int width, int height) : m_width(width), m_height(height) {
		m_window = SDL_CreateWindow(title,width, height, SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE);
		if (!m_window) {
			throw std::runtime_error(std::string("Window error: ") + SDL_GetError());
		}
	}
	Window::~Window() {
		if (m_window) {
			SDL_DestroyWindow(m_window);
		}

		SDL_Quit();
	}

	void Window::init(const char* title, int width, int height) {
		m_window = SDL_CreateWindow(title, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
		if (!m_window) {
			throw std::runtime_error(std::string("Window error: ") + SDL_GetError());
		}
	}
}