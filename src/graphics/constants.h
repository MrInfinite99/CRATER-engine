#pragma once
#include <cstdint>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
const std::string shader_path = "D:/vkguide/VkRE/shaders/basic.slang";
constexpr const char* texture_path = "D:/vkguide/VkRE/textures/viking_room.png";
constexpr const char* model_path = "D:/vkguide/VkRE/models/viking_room.obj";
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};