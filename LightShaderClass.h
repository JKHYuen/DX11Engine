#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <fstream>
using namespace DirectX;

class LightClass;

// constexpr int g_numLights = 4;

class LightShaderClass {
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
        float heightMapScale;
    };

    struct MaterialParamBufferType {
        float uvScale;
        XMFLOAT3 padding;
    };

    struct LightBufferType {
        XMFLOAT4 diffuseColor;
        XMFLOAT3 lightDirection;
        float time;
    };

public:
    LightShaderClass();
    LightShaderClass(const LightShaderClass&);
    ~LightShaderClass();

    bool Initialize(ID3D11Device*, HWND);
    void Shutdown();
    bool Render(ID3D11DeviceContext* deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ID3D11ShaderResourceView* albedoMap, ID3D11ShaderResourceView* normalMap, ID3D11ShaderResourceView* metallicMap, ID3D11ShaderResourceView* roughnessMap, ID3D11ShaderResourceView* aoMap, ID3D11ShaderResourceView* heightMap, ID3D11ShaderResourceView* shadowMap, LightClass* light, XMFLOAT3 cameraPosition, float time, float uvScale, float heightMapScale);

private:
    bool InitializeShader(ID3D11Device*, HWND, WCHAR*, WCHAR*);
    void OutputShaderErrorMessage(ID3D10Blob*, HWND, WCHAR*);

    void RenderShader(ID3D11DeviceContext*, int);

private:
    ID3D11VertexShader* m_vertexShader {};
    ID3D11PixelShader* m_pixelShader {};
    ID3D11InputLayout* m_layout {};
    ID3D11SamplerState* m_sampleStateWrap {};
    ID3D11SamplerState* m_sampleStateClamp {};
    ID3D11Buffer* m_matrixBuffer {};

    ID3D11Buffer* m_cameraBuffer {};
    ID3D11Buffer* m_materialParamBuffer {};
    ID3D11Buffer* m_lightBuffer {};

    ID3D11Buffer* m_lightColorBuffer {};
    ID3D11Buffer* m_lightPositionBuffer {};
};