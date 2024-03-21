cbuffer MatrixBuffer {
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

cbuffer CameraBuffer {
    float3 cameraPosition;
    float padding;
};

struct VertexInputType {
    float4 position : POSITION;
    float2 uv       : TEXCOORD0;
    float3 normal   : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
};

struct PixelInputType {
    float4 position      : SV_POSITION;
    float2 uv            : TEXCOORD0;
    float3 viewDirection : TEXCOORD1;
    float3 normal        : NORMAL;
    float3 tangent       : TANGENT;
    float3 binormal      : BINORMAL;
};

PixelInputType LightVertexShader(VertexInputType input) {
    PixelInputType output;
    float4 worldPosition;

    // Change the position vector to be 4 units for proper matrix calculations.
    input.position.w = 1.0f;

    // Calculate the position of the vertex against the world, view, and projection matrices.
    output.position = mul(input.position, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    
    // Store the texture coordinates for the pixel shader.
    output.uv = input.uv;
    
    // Calculate the position of the vertex in the world.
    worldPosition = mul(input.position, worldMatrix);

    // Determine the viewing direction based on the position of the camera and the position of the vertex in the world.
    output.viewDirection = normalize(cameraPosition.xyz - worldPosition.xyz);
    
    // TBN
    output.normal = normalize(mul(input.normal, (float3x3) worldMatrix));
    output.tangent = normalize(mul(input.tangent, (float3x3) worldMatrix));
    output.binormal = normalize(mul(input.binormal, (float3x3) worldMatrix));

    return output;
}