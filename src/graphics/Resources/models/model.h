#pragma once
#include<tiny_gltf.h>
 
#include<unordered_map>
#include"../Object/mesh.h"
#include"../Object/material.h"

#include"../resource_manager.h"


namespace CRATER::Resource{

    
    

	class Model:public Resource{
	public:

        // Local cleanup ONLY — never call Unload()/m_manager from the destructor.
        // At shutdown this destructor runs while the ResourceManager's cache is
        // itself being destroyed; Release-ing into that dying map is UB (the exit
        // stall). Runtime releases go through the manager, which calls Unload()
        // explicitly BEFORE dropping the resource — the cascade still happens there.
        ~Model() override {
            for (auto& td : m_textures) {
                if (td.ktx) {
                    ktxTexture_Destroy(ktxTexture(td.ktx));
                    td.ktx = nullptr;
                }
            }
        }
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
        const std::vector<MaterialData>& GetMaterials() const { return m_materials; }
        
        
        bool doLoad() override;
        void doUnload() override;
	private:
        TextureData loadKtxFromBufferView(const tinygltf::Model& model,
            int                    imageIndex,
            const std::string& name);

        bool parseMeshes(const tinygltf::Model& model);
        bool parseTextures(const tinygltf::Model& model);
        bool parseMaterials(const tinygltf::Model& model);

        
        std::string               m_path;
        ResourceManager* m_manager;
        Renderer::VulkanDevice* m_device;
        VmaAllocator      m_allocator;
        std::vector<MeshData> m_meshes;
        std::vector<TextureData> m_textures;
        std::vector<MaterialData> m_materials;
        std::vector<ResourceHandle<Mesh>>    meshData;
        std::vector<ResourceHandle<Texture>> textureData;
        
	};
};