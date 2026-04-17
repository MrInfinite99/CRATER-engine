#pragma once
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS 
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include<SDL3/SDL.h>
#include<SDL3/SDL_vulkan.h>

#include <iostream>
 
#include<vector>

namespace CRATER::Renderer
{
	class ValidationLayers {
	private:
		const std::vector<const char*> VALIDATION_LAYERS = {
			"VK_LAYER_KHRONOS_validation"
		};

		bool enableValidationLayers = true;


	public:
		ValidationLayers(bool enable) :enableValidationLayers{ enable } {};
		~ValidationLayers() = default;

		ValidationLayers() {
			enableValidationLayers = true;
		}

		void enable(bool toggle) {
			enableValidationLayers = toggle;
		}

		bool isEnabled() {
			return enableValidationLayers;
		}

		std::vector<char const*> getRequiredLayers(vk::raii::Context& context) {
			//get the required layers
			std::vector<char const*> requiredLayers;
			if (enableValidationLayers) {
				requiredLayers.assign(VALIDATION_LAYERS.begin(), VALIDATION_LAYERS.end());
			}

			// Check if the required layers are supported by the Vulkan implementation.
			auto layerProperties = context.enumerateInstanceLayerProperties();
			if (std::ranges::any_of(requiredLayers, [&layerProperties](auto const& requiredLayer) {
				return std::ranges::none_of(layerProperties,
					[requiredLayer](auto const& layerProperty)
					{ return strcmp(layerProperty.layerName, requiredLayer) == 0; });
				}))
			{
				throw std::runtime_error("One or more required layers are not supported!");
			}

			return requiredLayers;
		}

		std::vector<const char*> getExtensionList(vk::raii::Context& context) {
			uint32_t extensionCount = 0;

			char const* const* ppExtensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);

			// Check if the required extensions are supported by the Vulkan implementation.
			auto extensionProperties = context.enumerateInstanceExtensionProperties();
			// FIX: Use a traditional for loop with the count
			for (uint32_t i = 0; i < extensionCount; ++i)
			{
				const char* requiredExtension = ppExtensions[i]; // Access the element by index

				if (std::ranges::none_of(extensionProperties,
					[requiredExtension](auto const& extensionProperty)
					{
						return extensionProperty.extensionName == std::string_view(requiredExtension);
					}))
				{
					throw std::runtime_error("Required extension not supported: " + std::string(requiredExtension));
				}
			}
			std::vector<const char*> extensionsList(ppExtensions, ppExtensions + extensionCount);

			// ensure debug utils is enabled when validation layers are active
			if (enableValidationLayers) {
				extensionsList.push_back("VK_EXT_debug_utils");
			}

			return extensionsList;
		}

		

	};
}