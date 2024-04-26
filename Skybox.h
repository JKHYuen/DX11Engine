#pragma once
#include <vector>
#include <array>
#include <string>

#include <d3d11.h>
#include <directxmath.h>
using namespace DirectX;

class Texture;
class RenderTexture;
class Model;
class QuadModel;
class Camera;
class D3DInstance;

class Skybox {
public:
    enum RenderType {
        kHDRCaptureRender            = 0,
        kIrradianceConvolutionRender = 1,
        kSkyBoxRender                = 2,
        kPrefilterRender             = 3,
        kIntegrateBRDFRender         = 4,
        Num_RenderType
    };

public:
    Skybox() {}
    Skybox(const Skybox&) {}
    ~Skybox() {}

    bool Initialize(D3DInstance* d3dInstance, HWND hwnd, const std::string& fileName, int cubeFaceResolution, int cubeMapMipLevels, int irradianceMapResolution, int fullPrefilterMapResolution, int precomputedBRDFResolution, XMMATRIX screenDisplayViewMatrix, XMMATRIX screenOrthoMatrix, QuadModel* screenDisplayQuad);

    void Shutdown();
    bool Render(ID3D11DeviceContext* deviceContext, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, RenderType renderType, float roughness = 0);

    ID3D11ShaderResourceView* GetIrradianceMapSRV()   const;
    ID3D11ShaderResourceView* GetPrefilteredMapSRV()  const;
    ID3D11ShaderResourceView* GetPrecomputedBRDFSRV() const;

private:
    struct MatrixBufferType {
        XMMATRIX view;
        XMMATRIX projection;
    };

    struct PrefilterBufferType {
        float roughness;
        XMFLOAT3 padding;
    };

    struct VertexType {
        XMFLOAT3 position;
        XMFLOAT2 uv;
    };

private:
    static bool InitializeShader(ID3D11Device* device, HWND hwnd, std::wstring shaderName, ID3D11VertexShader** ppVertShader, ID3D11PixelShader** ppPixelShader);
    static bool InitializeUnitCubeBuffers(ID3D11Device* device);

    // Initialilze all resources shared between skybox instances
    bool InitializeStaticResources(D3DInstance* d3dInstance, HWND hwnd, int precomputedBRDFResolution, XMMATRIX screenDisplayViewMatrix, XMMATRIX screenOrthoMatrix, QuadModel* screenDisplayQuad);

private:
    static inline bool mb_StaticsInitialized {};
    static inline RenderTexture* m_PrecomputedBRDFTex {};

    /// Shaders
    static inline ID3D11VertexShader* m_CubeMapVertexShader {};
    static inline ID3D11PixelShader*  m_CubeMapPixelShader {};
    static inline ID3D11VertexShader* m_HDREquiVertexShader {};
    static inline ID3D11PixelShader*  m_HDREquiPixelShader {};
    static inline ID3D11VertexShader* m_ConvolutionVertexShader {};
    static inline ID3D11PixelShader*  m_ConvolutionPixelShader {};
    static inline ID3D11VertexShader* m_PrefilterVertexShader {};
    static inline ID3D11PixelShader*  m_PrefilterPixelShader {};
    static inline ID3D11VertexShader* m_IntegrateBRDFVertexShader{};
    static inline ID3D11PixelShader*  m_IntegrateBRDFPixelShader{};

    static inline ID3D11Buffer* m_MatrixBuffer {};
    static inline ID3D11Buffer* m_PrefilterParamBuffer {};
    static inline ID3D11Buffer* m_CubeVertexBuffer {};
    static inline ID3D11Buffer* m_CubeIndexBuffer {};
    static inline ID3D11InputLayout* m_Layout {};
    static inline ID3D11SamplerState* m_ClampSampleState {};
    /// 

    Texture* m_HDRCubeMapTex {};
    RenderTexture* m_CubeMapTex {};

    // IBL
    RenderTexture* m_IrradianceCubeMapTex {};
    RenderTexture* m_PrefilteredCubeMapTex {};
};