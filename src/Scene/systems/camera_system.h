#pragma once
#include<glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL3/SDL.h>
#include<algorithm>
 


namespace CRATER::Scene {
	enum class CameraMovement {
		FORWARD,
		BACKWARD,
		LEFT,
		RIGHT,
		UP,
		DOWN
	};



	class CameraSystem  {
	private:
		bool firstMouse = true;
		float lastX = 0.0f;
		float lastY = 0.0f;

		glm::vec3 position;
		glm::vec3 front;
		glm::vec3 up;
		glm::vec3 right;
		glm::vec3 worldUp;

		float yaw;
		float pitch;

		float movementSpeed;
		float mouseSensitivity;
		float scrollSensitivity;
		float zoom;

		void update( );


	public:
		CameraSystem()=default;

		void init(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
			float yaw = -90.0f,
			float pitch = 0.0f) {


			worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

			this->position = position;
			this->up = up;
			this->yaw = yaw;
			this->pitch = pitch;

			movementSpeed = 6.5f;
			mouseSensitivity = 0.05f;
			scrollSensitivity = 2.0f;
			zoom = 45.0f;

			update();

		}

		void processInput(SDL_Event& e, float deltaTime);

		glm::mat4 getViewMatrix() const;
		glm::mat4 getProjectionMatrix(float aspectRatio,float nearPlane =0.1f,float farPlane=100.0f) const;

		void processKeyboard(CameraMovement direction, float deltaTime);     // Keyboard-based translation
		void processMouseMovement(float xrel, float yrel);
		 


		glm::vec3 getPosition() const { return position; }
		glm::vec3 getFront() const { return front; }
		float getZoom() const { return zoom; }
	};
}