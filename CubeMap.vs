// Global
cbuffer MatrixBuffer {
    matrix viewMatrix;
    matrix projectionMatrix;
};

struct VertexInputType {
    float4 position : POSITION;
    float3 uv : TEXCOORD0;
};

struct PixelInputType {
    float4 position : SV_POSITION;
    float3 uv : TEXCOORD0;
};

PixelInputType CubeMapVertexShader(VertexInputType i) {
    PixelInputType o;
    
    o.uv = i.position.xyz;
    
    i.position.w = 1.0f;
    o.position = mul(i.position, viewMatrix);
    o.position = mul(o.position, projectionMatrix).xyww;
    
    return o;
}
