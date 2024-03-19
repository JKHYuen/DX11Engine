#pragma once
#include <vector>
#include <array>
#include <string>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
using namespace DirectX;

class TextureClass;
class RenderTextureClass;
class ModelClass;
class QuadModel;
class HDRTexture;
class CameraClass;
class D3DClass;

class CubeMapObject {
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
    CubeMapObject();
    CubeMapObject(const CubeMapObject&);
    ~CubeMapObject();

    bool Initialize(D3DClass* d3dInstance, HWND hwnd, const std::string& fileName, int cubeFaceResolution, int cubeMapMipLevels, int irradianceMapResolution, int fullPrefilterMapResolution, int precomputedBRDFResolution, XMMATRIX screenDisplayViewMatrix, XMMATRIX screenOrthoMatrix, QuadModel* screenDisplayQuad);

    void Shutdown();
    bool Render(ID3D11DeviceContext* deviceContext, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, RenderType renderType, float roughness = 0);

    ID3D11ShaderResourceView* GetIrradianceMapSRV() const;
    ID3D11ShaderResourceView* GetPrefilteredMapSRV() const;
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
    void OutputShaderErrorMessage(ID3D10Blob*, HWND, WCHAR*);
    bool InitializeShader(ID3D11Device* device, HWND hwnd, std::wstring shaderName, ID3D11VertexShader** ppVertShader, ID3D11PixelShader** ppPixelShader);
    bool InitializeUnitCubeBuffers(ID3D11Device* device);

private:
    /// Shaders
    ID3D11VertexShader* m_CubeMapVertexShader {};
    ID3D11PixelShader* m_CubeMapPixelShader {};

    ID3D11VertexShader* m_HDREquiVertexShader {};
    ID3D11PixelShader* m_HDREquiPixelShader {};

    ID3D11VertexShader* m_ConvolutionVertexShader {};
    ID3D11PixelShader* m_ConvolutionPixelShader {};

    ID3D11VertexShader* m_PrefilterVertexShader {};
    ID3D11PixelShader* m_PrefilterPixelShader {};

    ID3D11VertexShader* m_IntegrateBRDFVertexShader{};
    ID3D11PixelShader* m_IntegrateBRDFPixelShader{};

    ID3D11Buffer* m_VertexBuffer {};
    ID3D11Buffer* m_IndexBuffer {};
    ID3D11InputLayout* m_Layout {};
    ID3D11SamplerState* m_ClampSampleState {};
    /// 

    ID3D11Buffer* m_MatrixBuffer {};
    ID3D11Buffer* m_CameraBuffer {};
    ID3D11Buffer* m_PrefilterParamBuffer {};

    HDRTexture* m_HDRCubeMapTex {};
    RenderTextureClass* m_CubeMapTex {};

    // IBL
    RenderTextureClass* m_IrradianceCubeMapTex {};
    RenderTextureClass* m_PrefilteredCubeMapTex {};
    RenderTextureClass* m_PrecomputedBRDFTex {};
};