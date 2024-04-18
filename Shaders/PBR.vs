struct VertexInputType {
    float4 position : POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
};

struct HullInputType {
    float4 position : POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
};

HullInputType PBRVertexShader(VertexInputType i) {
    HullInputType o;
    o.position = i.position;
    o.uv = i.uv;
    o.normal = i.normal;
    o.tangent = i.tangent;
    o.binormal = i.binormal;
    return o;
}