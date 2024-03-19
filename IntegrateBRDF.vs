cbuffer MatrixBuffer {
    matrix viewMatrix;
    matrix projectionMatrix;
};

struct VertexInputType {
    float4 position : POSITION;
    float2 uv : TEXCOORD0;
};

struct PixelInputType {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

PixelInputType Vert(VertexInputType i) {
    PixelInputType o;
    
    o.uv = i.uv;
    
    i.position.w = 1.0f;
    o.position = mul(i.position, viewMatrix);
    o.position = mul(o.position, projectionMatrix);
    
    return o;
}
