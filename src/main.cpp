#include "Core/engine.h"
#include "engineTypes.h"
#include <iostream>
#include <exception>

int main() {
	CRATER::Scene::Scene scene;

	scene.initialize(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
	/*auto room = scene.CreateEntity();

	room.AddComponent< CRATER::Scene::TransformComponent>(
		glm::vec3(0.0f, 0.0f, -1.0f),
		glm::quat(0.0f, 0.0f, 0.0f, 0.0f),
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
	);*/

	auto room2 = scene.CreateEntity();

	room2.AddComponent< CRATER::Scene::TransformComponent>(
		glm::vec3(2.0f, 2.0f, 2.0f),
		glm::quat(0.0f, 0.0f, 0.0f, 0.0f),
		glm::vec3(1.0f)
	);

	room2.AddComponent<CRATER::Scene::MeshComponent>(
		"helmet",
		"D:/vkguide/VkRE/models/DamagedHelmet.glb"
	);

	room2.AddComponent<CRATER::Scene::MaterialComponent>(
		"helmet",
		"D:/vkguide/VkRE/shaders/basic.slang",
		 PipelineType::OpaqueMesh
	);

	auto skybox = scene.CreateEntity();
	skybox.AddComponent< CRATER::Scene::SkyboxComponent>(
		 "grasslands",
		"D:/vkguide/VkRE/skybox/grass2.ktx2"
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