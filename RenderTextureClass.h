#pragma once
#include <d3d11.h>
#include <directxmath.h>
using namespace DirectX;

class RenderTextureClass {
public:
    RenderTextureClass();
    RenderTextureClass(const RenderTextureClass&);
    ~RenderTextureClass();

    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, int textureWidth, int textureHeight, float nearZ, float farZ, DXGI_FORMAT textureFormat, float perspectiveFOV = XM_PIDIV4);
    void Shutdown();

    void SetRenderTarget(ID3D11DeviceContext*);
    void ClearRenderTarget(ID3D11DeviceContext*, float, float, float, float);
    ID3D11ShaderResourceView* GetTextureSRV();

    void GetProjectionMatrix(XMMATRIX&);
    void GetOrthoMatrix(XMMATRIX&);

    int GetTextureWidth() const { return m_TextureWidth; };
    int GetTextureHeight() const { return m_TextureHeight; };
    ID3D11Texture2D* GetTexture() const { return m_RenderTargetTexture; };

    void EnableAlphaBlending();
    void DisableAlphaBlending();

    void TurnZBufferOn();
    void TurnZBufferOff();

private:
    ID3D11DeviceContext* m_DeviceContext {};
    int m_TextureWidth {}, m_TextureHeight {};

    ID3D11Texture2D* m_RenderTargetTexture {};

    ID3D11RenderTargetView* m_RenderTargetView {};
    ID3D11ShaderResourceView* m_ShaderResourceView {};

    ID3D11Texture2D* m_DepthStencilBuffer {};
    ID3D11DepthStencilView* m_DepthStencilView {};

    D3D11_VIEWPORT m_Viewport {};

    XMMATRIX m_ProjectionMatrix {};
    XMMATRIX m_OrthoMatrix {};

    ID3D11DepthStencilState* m_DepthStencilState {};
    ID3D11DepthStencilState* m_DepthDisabledStencilState {};

    ID3D11BlendState* m_AlphaEnableBlendingState {};
    ID3D11BlendState* m_AlphaDisableBlendingState {};
};