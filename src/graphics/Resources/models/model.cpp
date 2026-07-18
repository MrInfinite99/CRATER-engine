#include "model.h"

namespace CRATER::Resource {

	bool Model::doLoad() {
		try {
			tinygltf::Model    gltf;
			tinygltf::TinyGLTF loader;
			std::string        err, warn;

			const bool ok = m_path.ends_with(".glb")
				? loader.LoadBinaryFromFile(&gltf, &err, &warn, m_path)
				: loader.LoadASCIIFromFile(&gltf, &err, &warn, m_path);

			if (!warn.empty()) std::cout << "[Model] glTF warning: " << warn << "\n";
			if (!err.empty())  std::cerr << "[Model] glTF error: " << err << "\n";
			if (!ok) throw std::runtime_error("failed to parse model :"+ m_path);

			if (!parseTextures(gltf)) {
				throw std::runtime_error("failed to parse texture :" + m_path);
			}
			if (!parseMeshes(gltf)) {
				throw std::runtime_error("failed to parse meshes :" + m_path);
			}
			if (!parseMaterials(gltf)) {
				throw std::runtime_error("failed to parse materials :" + m_path);
			}

			for (size_t i = 0; i < m_meshes.size(); i++) {
				std::string subMeshId = m_path + "/mesh_" + std::to_string(i);
				auto meshHandle = m_manager->Load<Mesh>(subMeshId, m_meshes[i], m_device, m_allocator);
				if (!meshHandle.IsValid())
					std::cerr << "[Model] Failed to create mesh " << i << " in " << m_path << "\n";
				meshData.push_back(std::move(meshHandle));
			}

			for (size_t i = 0; i < m_textures.size(); i++) {
				if (!m_textures[i].ktx) {
					textureData.push_back({});
					continue;
				}
				std::string texName  = m_textures[i].name.empty() ? std::to_string(i) : m_textures[i].name;
				std::string subTexId = m_path + "/tex_" + texName;
				auto texHandle = m_manager->Load<Texture>(subTexId, m_textures[i], m_allocator, m_device);
				if (!texHandle.IsValid())
					std::cerr << "[Model] Failed to upload texture " << texName << " in " << m_path << "\n";
				textureData.push_back(std::move(texHandle));
			}

			m_meshes.clear();
			m_textures.clear();

			const bool anyValidMesh = std::any_of(meshData.begin(), meshData.end(),
				[](const auto& h) { return h.IsValid(); });
			if (!anyValidMesh) {
				std::cerr << "[Model] No valid meshes loaded from " << m_path << "\n";
				return false;
			}

			return true;
		}
		catch(const std::exception& e){
			std::cerr << "[Model] doLoad failed (" << m_path << "): " << e.what() << "\n";
			return false;
		}

		
	}

	void Model::doUnload() {
		// Release every sub-resource this model loaded in doLoad — the mirror of its
		// own Load calls. This runs when the model's refcount hits zero, so the
		// cascade frees meshes/textures exactly when their owner goes away.
		// (Clearing the handle vectors alone frees nothing: handles are non-owning.)
		for (auto& h : meshData)
			if (h.IsValid()) m_manager->Release<Mesh>(h.GetId());
		for (auto& h : textureData)
			if (h.IsValid()) m_manager->Release<Texture>(h.GetId());

		meshData.clear();
		textureData.clear();

		m_meshes.clear();
		for (auto& td : m_textures) {
			if (td.ktx) {
				ktxTexture_Destroy(ktxTexture(td.ktx));
				td.ktx = nullptr;
			}
		}
		m_textures.clear();
	}

	bool Model::parseMeshes(const tinygltf::Model& model) {
		for (const auto& mesh : model.meshes) {
			for (const auto& prim : mesh.primitives) {
			  try {
				MeshData data;
				data.materialIndex = prim.material;   // -1 when absent

				// ── Positions (required) ─────────────────────────────────────
				const auto& posAcc = model.accessors[prim.attributes.at("POSITION")];
				const auto& posBV = model.bufferViews[posAcc.bufferView];
				const auto& posBuf = model.buffers[posBV.buffer];

				// Byte stride: glTF allows interleaved buffers
				const size_t posStride = posBV.byteStride > 0
					? posBV.byteStride : sizeof(float) * 3;

				// ── Tex coords (optional) ────────────────────────────────────
				const tinygltf::Accessor* uvAcc = nullptr;
				const tinygltf::BufferView* uvBV = nullptr;
				const tinygltf::Buffer* uvBuf = nullptr;
				size_t uvStride = sizeof(float) * 2;

				if (prim.attributes.count("TEXCOORD_0")) {
					uvAcc = &model.accessors[prim.attributes.at("TEXCOORD_0")];
					uvBV = &model.bufferViews[uvAcc->bufferView];
					uvBuf = &model.buffers[uvBV->buffer];
					uvStride = uvBV->byteStride > 0 ? uvBV->byteStride : uvStride;
				}

				// ── Normals ───────────────────────────────────────
				const tinygltf::Accessor* nrmAcc = nullptr;
				const tinygltf::BufferView* nrmBV = nullptr;
				const tinygltf::Buffer* nrmBuf = nullptr;
				size_t nrmStride = sizeof(float) * 3;

				if (prim.attributes.count("NORMAL")) {
					nrmAcc = &model.accessors[prim.attributes.at("NORMAL")];
					nrmBV = &model.bufferViews[nrmAcc->bufferView];
					nrmBuf = &model.buffers[nrmBV->buffer];
					nrmStride = nrmBV->byteStride > 0 ? nrmBV->byteStride : nrmStride;
				}

				// ── TANGENT ──────────────────────────────────────────────────
				const tinygltf::Accessor* tanAcc = nullptr;
				const tinygltf::BufferView* tanBV = nullptr;
				const tinygltf::Buffer* tanBuf = nullptr;
				size_t tanStride = sizeof(float) * 4;

				if (prim.attributes.count("TANGENT")) {
					tanAcc = &model.accessors[prim.attributes.at("TANGENT")];
					tanBV = &model.bufferViews[tanAcc->bufferView];
					tanBuf = &model.buffers[tanBV->buffer];
					tanStride = tanBV->byteStride > 0 ? tanBV->byteStride : tanStride;
				}

				const uint32_t baseVertex = static_cast<uint32_t>(data.vertices.size());
				data.vertices.reserve(posAcc.count);

				for (size_t i = 0; i < posAcc.count; ++i) {
					Vertex v{};

					// Position — flip Y for Vulkan NDC (glTF is Y-up, right-hand)
					const float* pos = reinterpret_cast<const float*>(
						posBuf.data.data() + posBV.byteOffset + posAcc.byteOffset + i * posStride);
					v.pos = { pos[0], -pos[1], pos[2] };

					// Tex coords
					if (uvAcc) {
						const float* uv = reinterpret_cast<const float*>(
							uvBuf->data.data() + uvBV->byteOffset + uvAcc->byteOffset + i * uvStride);
						v.texCoord = { uv[0], uv[1] };
					}

					// Normals (also needs Y flip to match position)
					if (nrmAcc) {
						const float* n = reinterpret_cast<const float*>(
							nrmBuf->data.data() + nrmBV->byteOffset + nrmAcc->byteOffset + i * nrmStride);
						v.normal = { n[0], -n[1], n[2] };
					}

					if (tanAcc) {
						const float* t = reinterpret_cast<const float*>(
							tanBuf->data.data() + tanBV->byteOffset + tanAcc->byteOffset + i * tanStride);
						v.tangent = { t[0],-t[1],t[2],t[3] };
					}

					data.vertices.push_back(v);
				}
				//



				// ── Indices ──────────────────────────────────────────────────
				if (prim.indices >= 0) {
					const auto& idxAcc = model.accessors[prim.indices];
					const auto& idxBV = model.bufferViews[idxAcc.bufferView];
					const auto& idxBuf = model.buffers[idxBV.buffer];

					const uint8_t* raw = idxBuf.data.data()
						+ idxBV.byteOffset
						+ idxAcc.byteOffset;

					data.indices.reserve(idxAcc.count);

					for (size_t i = 0; i < idxAcc.count; ++i) {
						uint32_t idx = 0;
						switch (idxAcc.componentType) {
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
							idx = raw[i];
							break;
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
							idx = reinterpret_cast<const uint16_t*>(raw)[i];
							break;
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
							idx = reinterpret_cast<const uint32_t*>(raw)[i];
							break;
						default:
							throw std::runtime_error("[Model] Unsupported index component type");
						}
						data.indices.push_back(baseVertex + idx);
					}
				}

				m_meshes.push_back(std::move(data));
			  }
			  catch (const std::exception& e) {
				std::cerr << "[Model] Skipping primitive: " << e.what() << "\n";
			  }
			}
		}
		return true;
	}

	bool Model::parseTextures(const tinygltf::Model& model) {
		m_textures.reserve(model.textures.size());

		for (size_t i = 0; i < model.textures.size(); ++i) {
			const auto& tex = model.textures[i];
			const int   imgIndex = tex.source;

			if (imgIndex < 0 || imgIndex >= static_cast<int>(model.images.size())) {
				std::cerr << "[Model] Texture " << i << " has invalid source index\n";
				m_textures.push_back({});
				continue;
			}

			const auto& img = model.images[imgIndex];
			const std::string label = img.uri.empty()
				? ("texture_" + std::to_string(i)) : img.uri;

			// ── Path A: Raw PNG/JPG image decoded by tinygltf ────────
			// CHECK THIS FIRST. If tinygltf parsed the PNG/JPG out of the .glb buffer,
			// we use the decoded pixel data.
			if (!img.image.empty()) {
				std::vector<unsigned char> rgbaConverted;
				const unsigned char* sourceData = img.image.data();

				// Standard glTF uses 3 (RGB) or 4 (RGBA) components. We pad RGB up to RGBA.
				if (img.component == 3) {
					rgbaConverted.resize(img.width * img.height * 4);
					for (size_t y = 0; y < img.height; ++y) {
						for (size_t x = 0; x < img.width; ++x) {
							size_t srcIdx = (y * img.width + x) * 3;
							size_t dstIdx = (y * img.width + x) * 4;
							rgbaConverted[dstIdx] = img.image[srcIdx];
							rgbaConverted[dstIdx + 1] = img.image[srcIdx + 1];
							rgbaConverted[dstIdx + 2] = img.image[srcIdx + 2];
							rgbaConverted[dstIdx + 3] = 255; // Fully opaque alpha
						}
					}
					sourceData = rgbaConverted.data();
				}

				ktxTextureCreateInfo info{};
				info.vkFormat = VK_FORMAT_R8G8B8A8_SRGB;
				info.baseWidth = static_cast<uint32_t>(img.width);
				info.baseHeight = static_cast<uint32_t>(img.height);
				info.baseDepth = 1;
				info.numDimensions = 2;
				info.numLevels = 1;
				info.numLayers = 1;
				info.numFaces = 1;
				info.isArray = KTX_FALSE;
				info.generateMipmaps = KTX_FALSE;

				ktxTexture2* ktx = nullptr;
				KTX_error_code res = ktxTexture2_Create(
					&info, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &ktx);

				if (res != KTX_SUCCESS) {
					std::cerr << "[Model] ktxTexture2_Create failed: " << ktxErrorString(res) << "\n";
					m_textures.push_back({});
					continue;
				}

				// Copy RGBA pixels into level 0
				const size_t pixelBytes = static_cast<size_t>(img.width)
					* static_cast<size_t>(img.height) * 4;
				ktxTexture_SetImageFromMemory(
					ktxTexture(ktx), 0, 0, 0, sourceData, pixelBytes);

				TextureData td;
				td.ktx = ktx;
				td.width = info.baseWidth;
				td.height = info.baseHeight;
				td.levels = 1;
				td.layers = 1;
				td.format = vk::Format::eR8G8B8A8Srgb;
				td.needsTranscode = false;
				td.name = label;

				m_textures.push_back(std::move(td));
				continue;
			}

			// ── Path B: KTX2 loaded from a buffer view ───────────────────────
			// If image is empty, but a bufferView exists, it means tinygltf couldn't 
			// decode it natively. This happens with KTX2 compressed textures.
			if (img.bufferView >= 0) {
				m_textures.push_back(loadKtxFromBufferView(model, imgIndex, label));
				continue;
			}

			std::cerr << "[Model] Texture " << i << " has no usable data\n";
			m_textures.push_back({});
		}
		return true;
	}

	TextureData Model::loadKtxFromBufferView(const tinygltf::Model& model,
		int imageIndex, const std::string& name)
	{
		const auto& img = model.images[imageIndex];
		const auto& bv = model.bufferViews[img.bufferView];
		const auto& buf = model.buffers[bv.buffer];

		const uint8_t* rawData = buf.data.data() + bv.byteOffset;
		const size_t   rawBytes = bv.byteLength;

		ktxTexture2* ktx = nullptr;
		KTX_error_code res = ktxTexture2_CreateFromMemory(
			rawData, rawBytes, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx);

		if (res != KTX_SUCCESS) {
			std::cerr << "[Model] ktxTexture2_CreateFromMemory failed ("
				<< name << "): " << ktxErrorString(res) << "\n";
			return {};
		}

		TextureData td;
		td.ktx = ktx;
		td.width = ktx->baseWidth;
		td.height = ktx->baseHeight;
		td.levels = ktx->numLevels;
		td.layers = ktx->numLayers;
		td.name = name;

		if (ktxTexture2_NeedsTranscoding(ktx)) {
			td.needsTranscode = true;
			td.format = vk::Format::eUndefined;
		}
		else {
			td.needsTranscode = false;
			td.format = static_cast<vk::Format>(ktx->vkFormat);
		}

		return td;
	}


	bool Model::parseMaterials(const tinygltf::Model& model) {
		m_materials.reserve(model.materials.size());

		for (const auto& mat : model.materials) {
			MaterialData md{};
			const auto& pbr = mat.pbrMetallicRoughness;

			const auto& bcf = pbr.baseColorFactor;
			md.baseColorFactor = { (float)bcf[0], (float)bcf[1], (float)bcf[2], (float)bcf[3] };

			if (pbr.baseColorTexture.index >= 0)
				md.albedoIndex = pbr.baseColorTexture.index;

			// ── Metallic / Roughness ─────────────────────────────────────────
			md.metallicFactor = (float)pbr.metallicFactor;
			md.roughnessFactor = (float)pbr.roughnessFactor;

			if (pbr.metallicRoughnessTexture.index >= 0)
				md.metallicRoughnessIndex = pbr.metallicRoughnessTexture.index;

			// ── Normal map ───────────────────────────────────────────────────
			if (mat.normalTexture.index >= 0) {
				md.normalIndex = mat.normalTexture.index;
				md.normalScale = (float)mat.normalTexture.scale;  // default 1.0
			}

			// ── Occlusion ────────────────────────────────────────────────────
			if (mat.occlusionTexture.index >= 0) {
				md.occlusionIndex = mat.occlusionTexture.index;
				md.occlusionStrength = (float)mat.occlusionTexture.strength;  // default 1.0
			}

			// ── Emissive ─────────────────────────────────────────────────────
			const auto& ef = mat.emissiveFactor;
			md.emissiveFactor = { (float)ef[0], (float)ef[1], (float)ef[2] };

			if (mat.emissiveTexture.index >= 0)
				md.emissiveIndex = mat.emissiveTexture.index;

			// ── Alpha mode ───────────────────────────────────────────────────
			if (mat.alphaMode == "BLEND")        md.alphaMode = AlphaMode::Blend;
			else if (mat.alphaMode == "MASK")    md.alphaMode = AlphaMode::Mask;
			else                                 md.alphaMode = AlphaMode::Opaque;

			md.alphaCutoff = (float)mat.alphaCutoff;
			md.doubleSided = mat.doubleSided;

			m_materials.push_back(std::move(md));
		}
		return true;
	}
}