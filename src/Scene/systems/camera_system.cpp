
#include"camera_system.h"

namespace CRATER::Scene {
    void CameraSystem::update( ) {

        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(newFront);

        // Recalculate the right and up vectors
        right = glm::normalize(glm::cross(front, worldUp));
        up = glm::normalize(glm::cross(right, front));
    }


    glm::mat4 CameraSystem::getViewMatrix() const {
        return glm::lookAt(position, position + front, up);
    }

    glm::mat4 CameraSystem::getProjectionMatrix(float aspectRatio, float nearPlane, float farPlane) const {
        return glm::perspective(glm::radians(zoom), aspectRatio, nearPlane, farPlane);
    }

	void CameraSystem::processKeyboard(CameraMovement direction,float deltaTime) {
		float velocity = movementSpeed * deltaTime;

		switch (direction) {
		case CameraMovement::FORWARD:
			position += front * velocity;
			break;
        case CameraMovement::BACKWARD:
            position -= front * velocity;
            break;
        case CameraMovement::LEFT:
            position -= right * velocity;
            break;
        case CameraMovement::RIGHT:
            position += right * velocity;
            break;
        case CameraMovement::UP:
            position += up * velocity;
            break;
        case CameraMovement::DOWN:
            position -= up * velocity;
            break;
		}
	}

    void CameraSystem::processInput(SDL_Event& e, float deltaTime) {
        const bool* keys = SDL_GetKeyboardState(nullptr);

        if (keys[SDL_SCANCODE_W]) processKeyboard(CameraMovement::FORWARD, deltaTime);
        if (keys[SDL_SCANCODE_S]) processKeyboard(CameraMovement::BACKWARD, deltaTime);
        if (keys[SDL_SCANCODE_A]) processKeyboard(CameraMovement::LEFT, deltaTime);
        if (keys[SDL_SCANCODE_D]) processKeyboard(CameraMovement::RIGHT, deltaTime);
        if (keys[SDL_SCANCODE_SPACE]) processKeyboard(CameraMovement::UP, deltaTime);
        if (keys[SDL_SCANCODE_LCTRL]) processKeyboard(CameraMovement::DOWN, deltaTime);

        if (e.type == SDL_EVENT_MOUSE_MOTION) {

            processMouseMovement(e.motion.x, e.motion.y);
        }
        if (e.type == SDL_EVENT_MOUSE_WHEEL) {
            zoom -= e.wheel.y*scrollSensitivity;

            if (zoom < 1.0f)  zoom = 1.0f;
            if (zoom > 90.0f) zoom = 90.0f;
        }
    }

    void CameraSystem::processMouseMovement(float xpos, float ypos) {
        
 
        if (firstMouse) {
            lastX = xpos;               
            lastY = ypos;
            firstMouse = false;         
        }

        float xOffset = xpos - lastX;                   // Horizontal movement (left-right)
        float yOffset = lastY - ypos;                   // Vertical movement (inverted: screen Y increases downward, camera pitch increases upward)

        xOffset *= mouseSensitivity;
        yOffset *= mouseSensitivity;

        // Update state for next callback iteration
        lastX = xpos;
        lastY = ypos;

        yaw += xOffset;
        pitch += yOffset;

        pitch = std::clamp(pitch, -89.0f, 89.0f);

        // Update camera vectors based on updated Euler angles
        update( );
    }


}