struct VSInput {
    float3 inPos;
    float3 inColor;
    float2 inTexCoord;
};

 struct UniformBuffer {
    column_major float4x4 view;
    column_major float4x4 proj;
};
ConstantBuffer<UniformBuffer> ubo;

Sampler2D texture;

struct PushConstants{
    column_major float4x4 model;
};

[[vk::push_constant]] PushConstants pushData;

struct VSOutput
{
    float4 pos : SV_Position;
    float3 fragColor;
    float2 fragTexCoord;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    
    // Standard MVP transformation
    float4 worldPos = mul(pushData.model, float4(input.inPos, 1.0));
    float4 viewPos  = mul(ubo.view, worldPos);
    output.pos      = mul(ubo.proj, viewPos);
  
    output.fragColor = input.inColor; // Assuming you have color
    output.fragTexCoord = input.inTexCoord;
    return output;
}



[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
     return texture.Sample(vertIn.fragTexCoord);

     
  }