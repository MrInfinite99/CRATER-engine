# CMakeList.txt : CMake project for VkRE, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.25)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("CRATER")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)
find_package(SDL3  CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(imgui  CONFIG REQUIRED)
find_package(glm  CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(SPIRV-Headers CONFIG REQUIRED)

# Add source to this project's executable.
 

 set(CORE
    src/Core/engine.cpp
    src/Core/window.cpp
)

set(RENDERER
    src/graphics/Renderer/Device.cpp
    src/graphics/Renderer/graphics_pipeline.cpp 
    src/graphics/Renderer/renderer.cpp 
    src/graphics/Renderer/swap_chain.cpp 
    src/graphics/Renderer/descriptor_set.cpp 
     
)

set(RESOURCES 
    src/graphics/Resources/shaders/shader_compile.cpp 
    src/graphics/Resources/vertex_buffer.cpp  
    src/graphics/Resources/index_buffer.cpp  
    src/graphics/Resources/textures/texture.cpp  
      
)

set(SCENE
    src/Scene/Scene.cpp
    src/Scene/RenderSystem.cpp
)

set(EXTERNAL
    src/external/vma.cpp
    src/external/spirv_reflect.cpp 
    src/external/stb_image.cpp
    src/external/tiny_obj_loader.cpp 
)


add_executable(${PROJECT_NAME}
    src/main.cpp
    ${CORE}
    ${RENDERER}
    ${RESOURCES}
    ${SCENE}
    ${EXTERNAL}
)


target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
   
)

# Link all required libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Vulkan::Vulkan
    
        SDL3::SDL3
        glm::glm
        imgui::imgui
        EnTT::EnTT
        GPUOpen::VulkanMemoryAllocator 
        SPIRV-Headers::SPIRV-Headers
)



 
