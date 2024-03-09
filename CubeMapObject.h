#pragma once
#include <vector>
#include <string>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
using namespace DirectX;

class TextureClass;
class ModelClass;
class HDRTexture;

class CubeMapObject {
private:
    struct MatrixBufferType {
        XMMATRIX view;
        XMMATRIX projection;
    };

    struct VertexType {
        XMFLOAT3 position;
        XMFLOAT2 uv;
    };

    static constexpr inline int k_UnitCubeVertexCount = 36;
    static constexpr inline int k_UnitCubeIndexCount = 36;
    static constexpr float k_UnitCubeVertices[] = {
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

public:
    CubeMapObject();
    CubeMapObject(const CubeMapObject&);
    ~CubeMapObject();

    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND hwnd, const std::string& fileName, int cubeFaceResolution);

    void Shutdown();
    bool Render(ID3D11DeviceContext* deviceContext, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, bool isSkyBoxRender = true);

private:
    void OutputShaderErrorMessage(ID3D10Blob*, HWND, WCHAR*);
    bool InitializeShader(ID3D11Device* device, HWND hwnd, std::wstring shaderName, ID3D11VertexShader** ppVertShader, ID3D11PixelShader** ppPixelShader);
    bool InitializeUnitCubeBuffers(ID3D11Device* device);

private:
    ID3D11VertexShader* m_CubeMapVertexShader {};
    ID3D11PixelShader* m_CubeMapPixelShader {};

    ID3D11VertexShader* m_HDREquiVertexShader{};
    ID3D11PixelShader* m_HDREquiPixelShader{};

    ID3D11Buffer* m_VertexBuffer{};
    ID3D11Buffer* m_IndexBuffer{};
    ID3D11InputLayout* m_Layout {};
    ID3D11SamplerState* m_ClampSampleState {};

    ID3D11Buffer* m_MatrixBuffer {};
    ID3D11Buffer* m_CameraBuffer {};
    ID3D11Buffer* m_MaterialParamBuffer {};


    TextureClass* m_CubeMapTex {};
    HDRTexture* m_HDRCubeMapTex {};
};