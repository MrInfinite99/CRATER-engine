#pragma once
#include <cstdint>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

struct UniformBufferObject {
    glm::mat4 view;
    glm::mat4 proj;
  // glm::vec4 lightPositions[4];           // Light positions in world space
  // glm::vec4 lightColors[4];              // Light intensities and colors
    glm::vec3 camPos;                      // Camera position for view-dependent effects
  // float exposure;                     // HDR exposure control
  // float gamma;                        // Gamma correction value (typically 2.2)
  // float prefilteredCubeMipLevels;     // IBL prefiltered environment map mip levels
  // float scaleIBLAmbient;
};

struct  StandardPushConstants {
    glm::mat4 model; 
    glm::vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
   // int baseColorTextureSet;
   // int physicalDescriptorTextureSet;
   // int normalTextureSet;
  //  int occlusionTextureSet;
  //  int emissiveTextureSet;
   // float alphaMask;
    float alphaMaskCutoff;
};