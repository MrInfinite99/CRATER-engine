

namespace CRATER {
	class Engine {
	public:
		void run() {
			init();
			mainLoop();
			cleanup();
		}

	private:
		void init();
		void mainLoop();
		void cleanup();
	};
}