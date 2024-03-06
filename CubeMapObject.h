#pragma once
#include <vector>
#include <string>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
using namespace DirectX;

class TextureClass;
class ModelClass;

class CubeMapObject {
private:
    struct MatrixBufferType {
        XMMATRIX view;
        XMMATRIX projection;
    };

public:
    CubeMapObject();
    CubeMapObject(const CubeMapObject&);
    ~CubeMapObject();

    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND hwnd, const std::string& textureFolderName);

    void Shutdown();
    bool Render(ID3D11DeviceContext*, XMMATRIX, XMMATRIX);

private:
    void OutputShaderErrorMessage(ID3D10Blob*, HWND, WCHAR*);
    bool InitializeShader(ID3D11Device*, HWND, WCHAR*, WCHAR*);

private:
    ID3D11VertexShader* m_VertexShader {};
    ID3D11PixelShader* m_PixelShader {};
    ID3D11InputLayout* m_Layout {};
    ID3D11SamplerState* m_SampleState {};
    ID3D11Buffer* m_MatrixBuffer {};

    ID3D11Buffer* m_CameraBuffer {};
    ID3D11Buffer* m_MaterialParamBuffer {};

    TextureClass* m_CubeMapTex {};
    ModelClass* m_UnitCubeModel {};
};