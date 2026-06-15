#pragma once
#include<tiny_gltf.h>
 
#include<unordered_map>
#include"../Object/mesh.h"
#include"../textures/texture.h"
#include"../resource_manager.h"


namespace CRATER::Resource{

    
    

	class Model:public Resource{
	public:

        ~Model() override { Unload(); }
        Model(const std::string& resourceId,
            const std::string& modelPath,
            ResourceManager* manager,
            Renderer::VulkanDevice* device,
            VmaAllocator allocator) :
            Resource(resourceId),
            m_path(modelPath),
            m_manager(manager),
            m_device(device),
            m_allocator(allocator)
        {}

        const std::vector<ResourceHandle<Mesh>>& GetMeshes()   const { return meshData; }
        const std::vector<ResourceHandle<Texture>>& GetTextures() const { return textureData; }
        
        
        bool doLoad() override;
        void doUnload() override;
	private:
        TextureData loadKtxFromBufferView(const tinygltf::Model& model,
            int                    imageIndex,
            const std::string& name);

        void parseMeshes(const tinygltf::Model& model);
        void parseTextures(const tinygltf::Model& model);

        
        std::string               m_path;
        ResourceManager* m_manager;
        Renderer::VulkanDevice* m_device;
        VmaAllocator      m_allocator;
        std::vector<MeshData> m_meshes;
        std::vector<TextureData> m_textures;
        std::vector<ResourceHandle<Mesh>>    meshData;
        std::vector<ResourceHandle<Texture>> textureData;
	};
};