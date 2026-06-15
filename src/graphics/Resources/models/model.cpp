#include "model.h"

namespace CRATER::Resource {

    bool Model::doLoad() {
        tinygltf::Model    gltf;
        tinygltf::TinyGLTF loader;
        std::string        err, warn;

        const bool ok = m_path.ends_with(".glb")
            ? loader.LoadBinaryFromFile(&gltf, &err, &warn, m_path)
            : loader.LoadASCIIFromFile(&gltf, &err, &warn, m_path);

        if (!warn.empty()) std::cout << "[Model] glTF warning: " << warn << "\n";
        if (!err.empty())  std::cerr << "[Model] glTF error: " << err << "\n";
        if (!ok) return false;

        parseTextures(gltf);
        parseMeshes(gltf);

        for (size_t i = 0; i < m_meshes.size(); i++) {
            std::string subMeshId = this->m_path + "/mesh_" + std::to_string(i);

            // Manager creates the Vulkan buffers and returns a ref-counted handle
            auto meshHandle = m_manager->Load<Mesh>(subMeshId, m_meshes[i], m_device, m_allocator);
            meshData.push_back(std::move(meshHandle));
        }

        for (size_t i = 0; i < m_textures.size(); i++) {
            // Use the texture name from glTF if available, otherwise use index
            std::string texName = m_textures[i].name.empty() ? std::to_string(i) : m_textures[i].name;
            std::string subTexId = this->m_path + "/tex_" + texName;

            auto texHandle = m_manager->Load<Texture>(subTexId, m_textures[i], m_allocator, m_device);
            textureData.push_back(std::move(texHandle));
        }

        m_meshes.clear();
        m_textures.clear();

        return true;
    }

    void Model::doUnload() {
        // Drop the ResourceHandles; the ResourceManager handles Vulkan memory cleanup
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

    void Model::parseMeshes(const tinygltf::Model& model) {
        for (const auto& mesh : model.meshes) {
            for (const auto& prim : mesh.primitives) {

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
                    uvStride = uvBV->byteStride > 0 ? uvBV->byteStride : sizeof(float) * 2;
                }

                // ── Normals (optional) ───────────────────────────────────────
                const tinygltf::Accessor* nrmAcc = nullptr;
                const tinygltf::BufferView* nrmBV = nullptr;
                const tinygltf::Buffer* nrmBuf = nullptr;
                size_t nrmStride = sizeof(float) * 3;

                if (prim.attributes.count("NORMAL")) {
                    nrmAcc = &model.accessors[prim.attributes.at("NORMAL")];
                    nrmBV = &model.bufferViews[nrmAcc->bufferView];
                    nrmBuf = &model.buffers[nrmBV->buffer];
                    nrmStride = nrmBV->byteStride > 0 ? nrmBV->byteStride : sizeof(float) * 3;
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
                        // v.normal = { n[0], -n[1], n[2] };
                        (void)n;
                    }

                    v.color = { 1.0f, 1.0f, 1.0f };
                    data.vertices.push_back(v);
                }

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
        }
    }

    void Model::parseTextures(const tinygltf::Model& model) {
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
}