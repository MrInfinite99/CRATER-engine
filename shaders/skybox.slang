struct VSInput{
	float3 inPos;
};

 struct UniformBuffer {
    column_major float4x4 view;
    column_major float4x4 proj;
};
ConstantBuffer<UniformBuffer> ubo;

SamplerCube skyboxTexture;

 

struct VSOutput
{
 float4 pos:SV_Position;
 float3 viewDir;
}

[shader("vertex")]
VSOutput vertMain(VSInput input)
{
    VSOutput output;
    
    // Pass the local object space position as the texturing/view direction
    output.viewDir = input.inPos;
    
    // Strip translation from the view matrix so the skybox stays centered on the camera.
    // We cast to a 3x3 to remove the 4th row/column translation, then back to a 4x4.
    //float4x4 staticView = float4x4(float3x3(ubo.view));

    float4x4 staticView;
staticView[0] = float4(ubo.view[0].xyz, 0.0);
staticView[1] = float4(ubo.view[1].xyz, 0.0);
staticView[2] = float4(ubo.view[2].xyz, 0.0);
staticView[3] = float4(0.0, 0.0, 0.0, 1.0);
    
    // Transform position into clip space
    float4 clipPos = mul(ubo.proj, mul(staticView, float4(input.inPos, 1.0)));
    
    // The xyww trick: Forces depth buffer value to 1.0 (maximum depth).
    // Pair this with a Depth Function of LEQUAL in your engine for optimal performance.
    output.pos = clipPos.xyww;
    
    return output;
}

// --- Fragment Shader ---

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target
{
    // Sample the cube map using the interpolated 3D direction vector
    float3 color = skyboxTexture.Sample(normalize(input.viewDir)).rgb;
    
  return float4(color, 1.0);
    //return float4(1.0, 0.0, 0.0, 1.0); // should see red everywhere behind geometry
}
