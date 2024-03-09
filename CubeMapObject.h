#pragma once
#include <vector>
#include <array>
#include <string>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
using namespace DirectX;

class TextureClass;
class ModelClass;
class HDRTexture;

class CubeMapObject {
public:
    enum RenderType {
        kHDRCapture = 0,
        kConvolution = 1,
        kSkyBox = 2,
        Num_RenderType
    };

public:
    CubeMapObject();
    CubeMapObject(const CubeMapObject&);
    ~CubeMapObject();

    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND hwnd, const std::string& fileName, int cubeFaceResolution);

    void Shutdown();
    bool Render(ID3D11DeviceContext* deviceContext, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, RenderType renderType);

    ID3D11ShaderResourceView* GetIrradianceSRV() const;

private:
    struct MatrixBufferType {
        XMMATRIX view;
        XMMATRIX projection;
    };

    struct VertexType {
        XMFLOAT3 position;
        XMFLOAT2 uv;
    };

    static constexpr inline int kUnitCubeVertexCount = 36;
    static constexpr inline int kUnitCubeIndexCount = 36;
    static constexpr float kUnitCubeVertices[] = {
        -1.0 , 1.0, -1.0, 0.0 , 0.0,
         1.0 , 1.0, -1.0, 1.0 , 0.0,
        -1.0, -1.0, -1.0, 0.0,  1.0,
        -1.0, -1.0, -1.0, 0.0,  1.0,
         1.0 , 1.0, -1.0, 1.0 , 0.0,
         1.0 ,-1.0, -1.0, 1.0 , 1.0,
         1.0 , 1.0, -1.0, 0.0 , 0.0,
         1.0 , 1.0,  1.0, 1.0 , 0.0,
         1.0 ,-1.0, -1.0, 0.0 , 1.0,
         1.0 ,-1.0, -1.0, 0.0 , 1.0,
         1.0 , 1.0,  1.0, 1.0 , 0.0,
         1.0 ,-1.0,  1.0, 1.0 , 1.0,
         1.0 , 1.0,  1.0, 0.0 , 0.0,
        -1.0,  1.0,  1.0, 1.0,  0.0,
         1.0 ,-1.0,  1.0, 0.0 , 1.0,
         1.0 ,-1.0,  1.0, 0.0 , 1.0,
        -1.0,  1.0,  1.0, 1.0,  0.0,
        -1.0, -1.0,  1.0, 1.0,  1.0,
        -1.0,  1.0,  1.0, 0.0,  0.0,
        -1.0,  1.0, -1.0, 1.0,  0.0,
        -1.0, -1.0,  1.0, 0.0,  1.0,
        -1.0, -1.0,  1.0, 0.0,  1.0,
        -1.0,  1.0, -1.0, 1.0,  0.0,
        -1.0, -1.0, -1.0, 1.0,  1.0,
        -1.0,  1.0,  1.0, 0.0,  0.0,
         1.0 , 1.0,  1.0, 1.0 , 0.0,
        -1.0,  1.0, -1.0, 0.0,  1.0,
        -1.0,  1.0, -1.0, 0.0,  1.0,
         1.0 , 1.0,  1.0, 1.0 , 0.0,
         1.0 , 1.0, -1.0, 1.0 , 1.0,
        -1.0, -1.0, -1.0, 0.0,  0.0,
         1.0 ,-1.0, -1.0, 1.0 , 0.0,
        -1.0, -1.0,  1.0, 0.0,  1.0,
        -1.0, -1.0,  1.0, 0.0,  1.0,
         1.0 ,-1.0, -1.0, 1.0 , 0.0,
         1.0 ,-1.0,  1.0, 1.0 , 1.0,
    };

    // View matrices for the 6 different cube directions
    static constexpr inline XMFLOAT3 float3_000  { 0.0f,   0.0f,  0.0f };
    static constexpr inline XMFLOAT3 float3_100  { 1.0f,   0.0f,  0.0f };
    static constexpr inline XMFLOAT3 float3_010  { 0.0f,   1.0f,  0.0f };
    static constexpr inline XMFLOAT3 float3_n100 { -1.0f,  0.0f,  0.0f };
    static constexpr inline XMFLOAT3 float3_00n1 { 0.0f,   0.0f, -1.0f };
    static constexpr inline XMFLOAT3 float3_0n10 { 0.0f,  -1.0f,  0.0f };
    static constexpr inline XMFLOAT3 float3_001  { 0.0f,   0.0f,  1.0f };
    static const inline std::array<XMMATRIX, 6> kCubeMapCaptureViewMats = {
        XMMatrixLookAtLH(XMLoadFloat3(&float3_000), XMLoadFloat3(&float3_100),  XMLoadFloat3(&float3_010)),
        XMMatrixLookAtLH(XMLoadFloat3(&float3_000), XMLoadFloat3(&float3_n100), XMLoadFloat3(&float3_010)),
        XMMatrixLookAtLH(XMLoadFloat3(&float3_000), XMLoadFloat3(&float3_010),  XMLoadFloat3(&float3_00n1)),
        XMMatrixLookAtLH(XMLoadFloat3(&float3_000), XMLoadFloat3(&float3_0n10),	XMLoadFloat3(&float3_001)),
        XMMatrixLookAtLH(XMLoadFloat3(&float3_000), XMLoadFloat3(&float3_001),	XMLoadFloat3(&float3_010)),
        XMMatrixLookAtLH(XMLoadFloat3(&float3_000), XMLoadFloat3(&float3_00n1), XMLoadFloat3(&float3_010)),
    };

private:
    void OutputShaderErrorMessage(ID3D10Blob*, HWND, WCHAR*);
    bool InitializeShader(ID3D11Device* device, HWND hwnd, std::wstring shaderName, ID3D11VertexShader** ppVertShader, ID3D11PixelShader** ppPixelShader);
    bool InitializeUnitCubeBuffers(ID3D11Device* device);

private:
    ID3D11VertexShader* m_CubeMapVertexShader {};
    ID3D11PixelShader* m_CubeMapPixelShader {};

    ID3D11VertexShader* m_HDREquiVertexShader{};
    ID3D11PixelShader* m_HDREquiPixelShader{};

    ID3D11VertexShader* m_ConvolutionVertexShader{};
    ID3D11PixelShader* m_ConvolutionPixelShader{};

    ID3D11Buffer* m_VertexBuffer{};
    ID3D11Buffer* m_IndexBuffer{};
    ID3D11InputLayout* m_Layout {};
    ID3D11SamplerState* m_ClampSampleState {};

    ID3D11Buffer* m_MatrixBuffer {};
    ID3D11Buffer* m_CameraBuffer {};
    ID3D11Buffer* m_MaterialParamBuffer {};

    TextureClass* m_CubeMapTex {};
    HDRTexture* m_HDRCubeMapTex {};
    TextureClass* m_IrradianceCubeMapTex {};
};