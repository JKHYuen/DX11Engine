Texture2D heightMap : register(t0);
SamplerState Sampler : register(s0);

cbuffer MatrixBuffer {
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    matrix lightViewMatrix;
    matrix lightProjectionMatrix;
};

cbuffer CameraBuffer {
    float3 cameraPosition;
    float heightMapScale;
};

struct VertexInputType {
    float4 position : POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
};

struct PixelInputType {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 cameraPosition : TEXCOORD1;
    float4 lightViewPosition : TEXCOORD2;
    float4 worldPosition : TEXCOORD3;
    
    float3 tangentCameraPosition : TEXCOORD4;
    float3 tangentFragPosition : TEXCOORD5;
    
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
};

PixelInputType PBRVertexShader(VertexInputType i) {
    PixelInputType o;
    
    // Change the position vector to be 4 units for proper matrix calculations.
    i.position.w = 1.0f;
    
    // Vertex displacement
    if (heightMapScale != 0) {
        float height = heightMap.SampleLevel(Sampler, i.uv, 0).r;
        i.position.xyz += i.normal * height * heightMapScale;
    }
    
    // Calculate the position of the vertex against the world, view, and projection matrices.
    o.position = mul(i.position, worldMatrix);
    o.position = mul(o.position, viewMatrix);
    o.position = mul(o.position, projectionMatrix);
    
    // Store the texture coordinates for the pixel shader.
    o.uv = i.uv;
    
    o.worldPosition = mul(i.position, worldMatrix);
    o.cameraPosition = cameraPosition.xyz;
    
    // TBN
    o.tangent = normalize(mul(i.tangent, (float3x3) worldMatrix));
    o.binormal = normalize(mul(i.binormal, (float3x3) worldMatrix));
    o.normal = normalize(mul(i.normal, (float3x3) worldMatrix));
    
    float3x3 TBN = transpose(float3x3(o.tangent, o.binormal, o.normal));
    o.tangentCameraPosition = mul(cameraPosition.xyz, TBN);
    o.tangentFragPosition = mul(o.worldPosition.xyz, TBN);
    
    // Shadow Mapping
    o.lightViewPosition = mul(i.position, worldMatrix);
    o.lightViewPosition = mul(o.lightViewPosition, lightViewMatrix);
    o.lightViewPosition = mul(o.lightViewPosition, lightProjectionMatrix);

    return o;
}