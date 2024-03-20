#pragma once

#include <d3d11.h>
#include <directxmath.h>
using namespace DirectX;

class LightClass;

// constexpr int g_numLights = 4;

class PBRShaderClass {
private:
    struct MatrixBufferType {
        XMMATRIX world;
        XMMATRIX view;
        XMMATRIX projection;
        XMMATRIX lightView;
        XMMATRIX lightProjection;
    };

    //struct LightColorBufferType {
    //    XMFLOAT4 diffuseColor[g_numLights];
    //};

    //struct LightPositionBufferType {
    //    XMFLOAT4 lightPosition[g_numLights];
    //};

    struct CameraBufferType {
        XMFLOAT3 cameraPosition;
        float displacementHeightScale;
    };

    struct MaterialParamBufferType {
        float uvScale;
        float parallaxHeightScale;
        XMFLOAT2 padding;
    };

    struct LightBufferType {
        XMFLOAT4 diffuseColor;
        XMFLOAT3 lightDirection;
        float time;
    };

public:
    PBRShaderClass();
    PBRShaderClass(const PBRShaderClass&);
    ~PBRShaderClass();

    bool Initialize(ID3D11Device*, HWND);
    void Shutdown();
    bool Render(ID3D11DeviceContext* deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ID3D11ShaderResourceView* albedoMap, ID3D11ShaderResourceView* normalMap, ID3D11ShaderResourceView* metallicMap, ID3D11ShaderResourceView* roughnessMap, ID3D11ShaderResourceView* aoMap, ID3D11ShaderResourceView* heightMap, ID3D11ShaderResourceView* shadowMap, ID3D11ShaderResourceView* irradianceMap, ID3D11ShaderResourceView* prefilteredMap, ID3D11ShaderResourceView* BRDFLut, LightClass* light, XMFLOAT3 cameraPosition, float time, float uvScale, float heightMapScale, float parallaxHeightScale);

private:
    bool InitializeShader(ID3D11Device*, HWND, WCHAR*, WCHAR*);
    void OutputShaderErrorMessage(ID3D10Blob*, HWND, WCHAR*);

    void RenderShader(ID3D11DeviceContext*, int);

private:
    ID3D11VertexShader* m_vertexShader {};
    ID3D11PixelShader* m_pixelShader {};
    ID3D11InputLayout* m_layout {};
    ID3D11SamplerState* m_sampleStateWrap {};
    ID3D11SamplerState* m_sampleStateBorder {};
    ID3D11SamplerState* m_sampleStateClamp {};
    ID3D11Buffer* m_matrixBuffer {};

    ID3D11Buffer* m_cameraBuffer {};
    ID3D11Buffer* m_materialParamBuffer {};
    ID3D11Buffer* m_lightBuffer {};

    ID3D11Buffer* m_lightColorBuffer {};
    ID3D11Buffer* m_lightPositionBuffer {};
};