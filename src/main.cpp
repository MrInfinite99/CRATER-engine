#include "Core/engine.h"
#include <iostream>
#include <exception>

int main(){
	CRATER::Scene::Scene scene;
	
	scene.initialize(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 1.0f, 0.0f),-90.0f,0.0f);
	auto room = scene.CreateEntity();

	room.AddComponent< CRATER::Scene::TransformComponent>(
		glm::vec3(0.0f, 0.0f, -1.0f),
		glm::quat(0.0f,0.0f,0.0f,0.0f),
		glm::vec3(1.0f)
	);

	room.AddComponent<CRATER::Scene::MeshComponent>(
		"vikingModel",
		"D:/vkguide/VkRE/models/viking_room.obj"
	);

	room.AddComponent<CRATER::Scene::MaterialComponent>(
		"vikingRoom",
		"D:/vkguide/VkRE/shaders/basic.slang",
		"D:/vkguide/VkRE/textures/viking_room.png"
	);

	 
	CRATER::Engine engine;

	engine.init(scene);

	try {
		engine.run(scene);
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}