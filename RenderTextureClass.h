#pragma once
#include <d3d11.h>
#include <directxmath.h>
using namespace DirectX;

class RenderTextureClass {
public:
    RenderTextureClass();
    RenderTextureClass(const RenderTextureClass&);
    ~RenderTextureClass();

    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, int textureWidth, int textureHeight, float screenDepth, float screenNear, DXGI_FORMAT textureFormat = DXGI_FORMAT_R32G32B32A32_FLOAT);
    void Shutdown();

    void SetRenderTarget(ID3D11DeviceContext*);
    void ClearRenderTarget(ID3D11DeviceContext*, float, float, float, float);
    ID3D11ShaderResourceView* GetShaderResourceView();

    void GetProjectionMatrix(XMMATRIX&);
    void GetOrthoMatrix(XMMATRIX&);

    int GetTextureWidth();
    int GetTextureHeight();

    void EnableAlphaBlending();
    void DisableAlphaBlending();

    void TurnZBufferOn();
    void TurnZBufferOff();

private:
    ID3D11DeviceContext* m_deviceContext {};

    int m_textureWidth {}, m_textureHeight {};
    ID3D11Texture2D* m_renderTargetTexture {};
    ID3D11RenderTargetView* m_renderTargetView {};
    ID3D11ShaderResourceView* m_shaderResourceView {};
    ID3D11Texture2D* m_depthStencilBuffer {};
    ID3D11DepthStencilView* m_depthStencilView {};
    D3D11_VIEWPORT m_viewport {};
    XMMATRIX m_projectionMatrix {};
    XMMATRIX m_orthoMatrix {};

    ID3D11DepthStencilState* m_depthStencilState {};
    ID3D11DepthStencilState* m_depthDisabledStencilState {};

    ID3D11BlendState* m_alphaEnableBlendingState {};
    ID3D11BlendState* m_alphaDisableBlendingState {};
};