#pragma once

#include <d3d11.h>
#include <directxmath.h>
using namespace DirectX;

class FontShader {
private:
    struct MatrixBufferType {
        XMMATRIX world;
        XMMATRIX view;
        XMMATRIX projection;
    };

    struct PixelBufferType {
        XMFLOAT4 pixelColor;
    };

public:
    FontShader() {}
    FontShader(const FontShader&) {}
    ~FontShader() {}

    bool Initialize(ID3D11Device*, HWND);
    void Shutdown();
    bool Render(ID3D11DeviceContext*, int, XMMATRIX, XMMATRIX, XMMATRIX, ID3D11ShaderResourceView*, XMFLOAT4);

private:
    void RenderShader(ID3D11DeviceContext*, int) const;

private:
    ID3D11VertexShader* m_vertexShader {};
    ID3D11PixelShader*  m_pixelShader  {};
    ID3D11InputLayout*  m_layout       {};

    ID3D11Buffer*       m_matrixBuffer {};
    ID3D11Buffer*       m_pixelBuffer  {};
    ID3D11SamplerState* m_sampleState  {};
};