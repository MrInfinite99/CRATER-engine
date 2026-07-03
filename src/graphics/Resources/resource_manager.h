#pragma once
#include <typeindex>
#include <unordered_map>
#include <memory>
#include"resource.h"

namespace CRATER::Resource {
    class ResourceManager;

	template<typename T>
	class ResourceHandle {
	private:
		std::string resourceId;
		ResourceManager* resourceManager;
	public:
		ResourceHandle() = default;

		ResourceHandle(const std::string& id, ResourceManager* manager)
			: resourceId(id), resourceManager(manager) {
		}

        T* Get() const;

        bool IsValid() const;

        const std::string& GetId() const;

		// Convenience operators
        T* operator->() const;

        T& operator*() const;

        operator bool() const;


	};



	/*As the name suggests the manager only owns the cache and manages resource handles the loading 
	implementation is done by the resource type classes*/
    class ResourceManager {
    private:
        struct ResourceData {
            std::shared_ptr<Resource> resource;
            int refCount = 0;
        };

        // Partitioned cache: Type -> (ID -> ResourceData)
        // Allows "player" model and "player" texture to coexist safely.
        std::unordered_map<std::type_index, std::unordered_map<std::string, ResourceData>> cache;

    public:
        ResourceManager() = default;

        template<typename T, typename... Args>
        ResourceHandle<T> Load(const std::string& id, Args&&... args) {
            static_assert(std::is_base_of_v<Resource, T>, "T must derive from Resource");

            auto typeIdx = std::type_index(typeid(T));
            auto& typeCache = cache[typeIdx];

            // 1. Check if cached
            auto it = typeCache.find(id);
            if (it != typeCache.end()) {
                it->second.refCount++;
                return ResourceHandle<T>(id, this);
            }

            // 2. Create and load
            auto resource = std::make_shared<T>(id, std::forward<Args>(args)...);
            if (!resource->Load()) {
                return ResourceHandle<T>();
            }

            // 3. Store in the type-specific cache
            typeCache[id] = { resource, 1 };
            return ResourceHandle<T>(id, this);
        }

        template<typename T>
        T* GetResource(const std::string& id) {
            auto typeIt = cache.find(std::type_index(typeid(T)));
            if (typeIt != cache.end()) {
                auto resIt = typeIt->second.find(id);
                if (resIt != typeIt->second.end()) {
                    return static_cast<T*>(resIt->second.resource.get());
                }
            }
            return nullptr;
        }

        template<typename T>
        bool HasResource(const std::string& id) {
            auto typeIt = cache.find(std::type_index(typeid(T)));
            if (typeIt != cache.end()) {
                return typeIt->second.find(id) != typeIt->second.end();
            }
            return false;
        }

        // Now requires <T> so it knows WHICH "player" to release
        template<typename T>
        void Release(const std::string& id) {
            auto typeIt = cache.find(std::type_index(typeid(T)));
            if (typeIt == cache.end()) return;

            auto& typeCache = typeIt->second;
            auto resIt = typeCache.find(id);
            if (resIt == typeCache.end()) return;

            if (--resIt->second.refCount <= 0) {
                resIt->second.resource->Unload();
                typeCache.erase(resIt);
            }
        }

        // Returns a handle to an already-cached resource without re-loading it.
        template<typename T>
        ResourceHandle<T> GetHandle(const std::string& id) {
            return ResourceHandle<T>(id, this);
        }

        void UnloadAll() {
            for (auto& [type, typeCache] : cache) {
                for (auto& [id, data] : typeCache) {
                    data.resource->Unload();
                }
            }
            cache.clear();
        }
    };
 


    template<typename T>
    T* ResourceHandle<T>::Get() const{
        if (!resourceManager) return nullptr;
        return resourceManager->GetResource<T>(resourceId);
    }

    template<typename T>
    bool ResourceHandle<T>::IsValid() const {
        return resourceManager && resourceManager->HasResource<T>(resourceId);
    }

    template<typename T>
    const std::string& ResourceHandle<T>::GetId() const {
        return resourceId;
    }

    // Convenience operators
    template<typename T>
    T* ResourceHandle<T>::operator->() const {
        return Get();
    }

    template<typename T>
    T& ResourceHandle<T>::operator*() const {
        return *Get();
    }

    template<typename T>
    ResourceHandle<T>::operator bool() const {
        return IsValid();
    }
}


