cbuffer TessellationBuffer {
    float tessellationAmount;
    float3 padding;
};

struct HullInputType {
    float4 position : POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
};

struct ConstantOutputType {
    float edges[3] : SV_TessFactor;
    float inside : SV_InsideTessFactor;
};

struct HullOutputType {
    float4 position : POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
};

ConstantOutputType PBRPatchConstantFunction(InputPatch<HullInputType, 3> inputPatch, uint patchId : SV_PrimitiveID) {
    ConstantOutputType output;

    // Set the tessellation factors for the three edges of the triangle.
    output.edges[0] = tessellationAmount;
    output.edges[1] = tessellationAmount;
    output.edges[2] = tessellationAmount;

    // Set the tessellation factor for tessallating inside the triangle.
    output.inside = tessellationAmount;

    return output;
}

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("PBRPatchConstantFunction")]
HullOutputType PBRHullShader(InputPatch<HullInputType, 3> patch, uint pointId : SV_OutputControlPointID, uint patchId : SV_PrimitiveID) {
    HullOutputType o;

    o.position = patch[pointId].position;
    o.uv = patch[pointId].uv;
    o.normal = patch[pointId].normal;
    o.tangent = patch[pointId].tangent;
    o.binormal = patch[pointId].binormal;
    
    return o;
}