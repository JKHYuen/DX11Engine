#pragma once
#include "GameObject.h"

#include <d3d11.h>
#include <directxmath.h>
#include <vector>
#include <array>
using namespace DirectX;

class DirectionalLight;
class Texture;
class Skybox;
class Camera;

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
        XMFLOAT3 cameraPosition;

        XMMATRIX world;
        XMFLOAT4 cullPlanes[4];

        float cullBias;
        XMFLOAT2 screenDimensions;
        float padding;
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
    enum TessellationMode {
        kDisabledTess = 0,
        kUniformTess  = 1,
        kEdgeTess     = 2,
        Num_TessellationModes
    };

    static inline const std::vector<std::string> s_TessellationModeNames {"Disabled", "Uniform", "Edge"};

public:
    PBRShader() {}
    PBRShader(const PBRShader&) {}
    ~PBRShader() {}

    bool Initialize(ID3D11Device*, HWND);
    void Shutdown();
    bool Render(ID3D11DeviceContext* deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX projectionMatrix, const std::vector<Texture*> materialTextures, ID3D11ShaderResourceView* shadowMap, Skybox* skybox, DirectionalLight* light, Camera* camera, const std::array<XMFLOAT4, 6>& cullFrustum, float time, const GameObject::GameObjectData& gameObjectData);

private:
    bool InitializeHullShaders(ID3D11Device* device, const std::wstring& hsFileName, HWND hwnd);

private:
    ID3D11VertexShader* m_VertexShader {};
    ID3D11PixelShader*  m_PixelShader {};
    ID3D11DomainShader* m_DomainShader {};
    // Indexed by TesselationModes enum
    std::array<ID3D11HullShader*, TessellationMode::Num_TessellationModes> m_HullShaders {};

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