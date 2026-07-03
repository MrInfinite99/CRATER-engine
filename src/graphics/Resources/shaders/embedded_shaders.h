#pragma once

namespace CRATER::Resource {

// Embedded fallback — compiled if the .slang file is missing or fails to compile.
// Matches the current vertex layout (pos, normal, texCoord, tangent),
// current UBO (view, proj, camPos), and current StandardPushConstants.
inline constexpr const char* kEmbeddedBasicShader = R"slang(
struct VSInput {
    float3 inPos      : POSITION;
    float3 inNormal   : NORMAL;
    float2 inTexCoord : TEXCOORD0;
    float4 inTangent  : TANGENT;
};

struct UniformBuffer {
    column_major float4x4 view;
    column_major float4x4 proj;
    float3 camPos;
};
ConstantBuffer<UniformBuffer> ubo;

Sampler2D baseColorMap;

struct PushConstants {
    column_major float4x4 model;
    float4 baseColorFactor;
    float  metallicFactor;
    float  roughnessFactor;
    float  alphaMaskCutoff;
};
[[vk::push_constant]] PushConstants pushData;

struct VSOutput {
    float4 pos          : SV_Position;
    float2 fragTexCoord : TEXCOORD0;
    float3 fragNormal   : NORMAL;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    float4 worldPos     = mul(pushData.model, float4(input.inPos, 1.0));
    output.pos          = mul(ubo.proj, mul(ubo.view, worldPos));
    output.fragTexCoord = input.inTexCoord;
    output.fragNormal   = input.inNormal;
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
    float4 color = baseColorMap.Sample(vertIn.fragTexCoord);
    return color * pushData.baseColorFactor;
}
)slang";

} // namespace CRATER::Resource
