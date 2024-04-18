#pragma once

#include "GameObject.h"

#include <d3d11.h>
#include <directxmath.h>
#include <vector>
using namespace DirectX;

class DirectionalLight;
class Texture;

// constexpr int g_numLights = 4;

class PBRShader {
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

    struct TessellationBufferType {
        float tessellationFactor;
        XMFLOAT3 padding;
    };

    struct CameraBufferType {
        XMFLOAT3 cameraPosition;
        float displacementHeightScale;

        float uvScale;
        XMFLOAT3 padding;
    };

    struct MaterialParamBufferType {
        float parallaxHeightScale;
        float minRoughness;
        XMFLOAT2 padding;

        float useParallaxShadow;
        float minParallaxLayers;
        float maxParallaxLayers;
        float shadowBias;
    };

    struct LightBufferType {
        XMFLOAT4 diffuseColor;
        XMFLOAT3 lightDirection;
        float time;
    };

public:
    PBRShader() {}
    PBRShader(const PBRShader&) {}
    ~PBRShader() {}

    bool Initialize(ID3D11Device*, HWND);
    void Shutdown();
    bool Render(ID3D11DeviceContext* deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, const std::vector<Texture*> materialTextures, ID3D11ShaderResourceView* shadowMap, ID3D11ShaderResourceView* irradianceMap, ID3D11ShaderResourceView* prefilteredMap, ID3D11ShaderResourceView* BRDFLut, DirectionalLight* light, XMFLOAT3 cameraPosition, float time, const GameObject::GameObjectData& gameObjectData);

private:
    ID3D11VertexShader* m_VertexShader {};
    ID3D11PixelShader*  m_PixelShader {};
    ID3D11HullShader*   m_HullShader {};
    ID3D11DomainShader* m_DomainShader {};

    ID3D11InputLayout*  m_Layout {};
    ID3D11SamplerState* m_SampleStateWrap {};
    ID3D11SamplerState* m_SampleStateBorder {};
    ID3D11SamplerState* m_SampleStateClamp {};

    ID3D11Buffer* m_MatrixBuffer {};
    ID3D11Buffer* m_CameraBuffer {};
    ID3D11Buffer* m_MaterialParamBuffer {};
    ID3D11Buffer* m_LightBuffer {};
    ID3D11Buffer* m_TessellationBuffer {};

    //ID3D11Buffer* m_LightColorBuffer {};
    //ID3D11Buffer* m_LightPositionBuffer {};
};