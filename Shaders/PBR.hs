cbuffer TessellationBuffer {
    float tessellationAmount;
    float3 cameraPosition;
    matrix modelMatrix;
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

float CalcTessellationFactor(float3 vertexPosition1, float3 vertexPosition2) {
    float viewDistance = distance(cameraPosition, (vertexPosition1 + vertexPosition2) * 0.5);
    
    float edgeLength = distance(vertexPosition1, vertexPosition2);
    
    // TODO: make screen height not hardcoded
    return (edgeLength * 1080) / (tessellationAmount * viewDistance);

    //float viewDistanceFactor = 1.0 - smoothstep(0, 50, viewDistance);
    //return clamp((edgeLength * 1080) / (tessellationAmount * viewDistance) * viewDistanceFactor, 1, 64);
}

bool TriangleIsBelowClipPlane(float3 p0, float3 p1, float3 p2) {
    float4 plane = float4(1, 0, 0, 0);
    return
		dot(float4(p0, 1), plane) < 0 &&
		dot(float4(p1, 1), plane) < 0 &&
		dot(float4(p2, 1), plane) < 0;
}

bool TriangleIsCulled(float3 p0, float3 p1, float3 p2) {
    return
		TriangleIsBelowClipPlane(p0, p1, p2) ||
		TriangleIsBelowClipPlane(p0, p1, p2) ||
		TriangleIsBelowClipPlane(p0, p1, p2) ||
		TriangleIsBelowClipPlane(p0, p1, p2);
}

ConstantOutputType PBRPatchConstantFunction(InputPatch<HullInputType, 3> inputPatch, uint patchId : SV_PrimitiveID) {
    ConstantOutputType output;
    
    //output.edges[0] = output.edges[1] = output.edges[2] = output.inside = 1;
    //return output;
    
    float4 vertexPosition1 = float4(inputPatch[0].position.xyz, 1.0);
    vertexPosition1 = mul(vertexPosition1, modelMatrix);
    float4 vertexPosition2 = float4(inputPatch[1].position.xyz, 1.0);
    vertexPosition2 = mul(vertexPosition2, modelMatrix);
    float4 vertexPosition3 = float4(inputPatch[2].position.xyz, 1.0);
    vertexPosition3 = mul(vertexPosition3, modelMatrix);
    
    //if(TriangleIsCulled(vertexPosition1.xyz, vertexPosition2.xyz, vertexPosition3.xyz)) {
    //    output.edges[0] = output.edges[1] = output.edges[2] = output.inside = 0;
    //    return output;
    //}

    output.edges[0] = CalcTessellationFactor(vertexPosition2.xyz, vertexPosition3.xyz);
    output.edges[1] = CalcTessellationFactor(vertexPosition3.xyz, vertexPosition1.xyz);
    output.edges[2] = CalcTessellationFactor(vertexPosition1.xyz, vertexPosition2.xyz);

    output.inside = (output.edges[0] + output.edges[1] + output.edges[2]) * (1.0 / 3.0);

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