#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

namespace CRATER::Resource {
	class Resource {
	private:
		std::string resourceId;
		bool loaded = false;
	public:
		explicit Resource(const std::string& id) : resourceId(id) {}
		virtual ~Resource() = default;

		// Core resource identity and state access methods
		const std::string& GetId() const { return resourceId; }
		bool IsLoaded() const { return loaded; }

		// Virtual interface for resource-specific loading and unloading behavior
		virtual bool Load() {
			loaded = doLoad();
			return loaded;
		}

		virtual void Unload() {
			doUnload();
			loaded = false;
		}//template method pattern

		virtual bool doLoad() = 0;
		virtual void doUnload() = 0;

	};
}