struct VertexInputType {
    float4 position : POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
};

struct HullInputType {
    float4 position : POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
};

HullInputType DepthVertexShader(VertexInputType i) {
    HullInputType o;
    o.position = i.position;
    o.uv = i.uv;
    o.normal = i.normal;
    return o;
}