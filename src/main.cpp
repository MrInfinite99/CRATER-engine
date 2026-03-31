#include "Core/engine.h"
#include <iostream>
#include <exception>

int main(){
	CRATER::Engine engine;

	try {
		engine.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}