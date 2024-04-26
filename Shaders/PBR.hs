cbuffer TessellationBuffer {
    float tessellationAmount;
    float3 cameraPosition;
    matrix modelMatrix;
    float4 cullingPlanes[4];
    float cullBias;
    float2 screenDimensions;
    float padding;
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

// Edge tessellation based on: https://catlikecoding.com/unity/tutorials/advanced-rendering/tessellation/
// Note: Distance based tessellation can be improved by adding min and max distance with interpolation between these values
float CalcTessellationFactor(float3 vertexPosition1, float3 vertexPosition2) {
    float viewDistance = distance(cameraPosition, (vertexPosition1 + vertexPosition2) * 0.5);
    
    float edgeLength = distance(vertexPosition1, vertexPosition2);
    
    return (edgeLength * screenDimensions.y) / (tessellationAmount * viewDistance);
}

// TODO: add bias (vertex displacement scale)
bool TriangleIsBelowClipPlane(float3 p0, float3 p1, float3 p2, int cullPlaneIndex) {
    float4 plane = cullingPlanes[cullPlaneIndex];
    return
		dot(float4(p0, 1), plane) < 0 &&
		dot(float4(p1, 1), plane) < 0 &&
		dot(float4(p2, 1), plane) < 0;
}

bool TriangleIsCulled(float3 p0, float3 p1, float3 p2) {
    return
		TriangleIsBelowClipPlane(p0, p1, p2, 0) ||
		TriangleIsBelowClipPlane(p0, p1, p2, 1) ||
		TriangleIsBelowClipPlane(p0, p1, p2, 2) ||
		TriangleIsBelowClipPlane(p0, p1, p2, 3);
}

ConstantOutputType PBRPatchConstantFunction(InputPatch<HullInputType, 3> inputPatch, uint patchId : SV_PrimitiveID) {
    ConstantOutputType output;
    
    //output.edges[0] = output.edges[1] = output.edges[2] = output.inside = 1;
    //return output;
    
    float3 vertexPosition0 = mul(float4(inputPatch[0].position.xyz, 1.0), modelMatrix).xyz;
    float3 vertexPosition1 = mul(float4(inputPatch[1].position.xyz, 1.0), modelMatrix).xyz;
    float3 vertexPosition2 = mul(float4(inputPatch[2].position.xyz, 1.0), modelMatrix).xyz;
    
    if(TriangleIsCulled(vertexPosition0, vertexPosition1, vertexPosition2)) {
        output.edges[0] = output.edges[1] = output.edges[2] = output.inside = 0;
        return output;
    }

    output.edges[0] = CalcTessellationFactor(vertexPosition1, vertexPosition2);
    output.edges[1] = CalcTessellationFactor(vertexPosition2, vertexPosition0);
    output.edges[2] = CalcTessellationFactor(vertexPosition0, vertexPosition1);

    output.inside = (output.edges[0] + output.edges[1] + output.edges[2]) / 3.0;

    return output;
}

[domain("tri")]
[partitioning("fractional_odd")]
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