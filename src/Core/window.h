#pragma
#include <SDL3/SDL.h>
#include<SDL3/SDL_vulkan.h>
#include <stdexcept>


namespace CRATER {
	class Window {
	public:
		Window(const char* title, int width, int height);
		~Window();
		SDL_Window* getSDLWindow() const { return m_window; }
		int getWidth() const { return m_width; }
		int getHeight() const { return m_height; }

		void init(const char* title, int width, int height);
	private:
		SDL_Window* m_window;
		int m_width;
		int m_height;
	};
}