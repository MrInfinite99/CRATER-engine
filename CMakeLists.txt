# CMakeList.txt : CMake project for VkRE, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.25)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("VkRE")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(SDL3  CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(imgui  CONFIG REQUIRED)
find_package(glm  CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)


# Add source to this project's executable.
add_executable (VkRE "src/main.cpp" "src/Scene/Scene.cpp"  "src/Core/engine.h" "src/Core/engine.cpp" "src/Core/window.h" "src/Core/window.cpp" "src/Renderer/renderer.h" "src/Renderer/renderer.cpp" "src/Renderer/validationLayers.h"  "src/Renderer/swap_chain.h" "src/Renderer/swap_chain.cpp")

target_include_directories(VkRE PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
   
)

# Link all required libraries
target_link_libraries(VkRE
    PRIVATE
        Vulkan::Vulkan
    
        SDL3::SDL3
        glm::glm
        imgui::imgui
        EnTT::EnTT
        
)



 
