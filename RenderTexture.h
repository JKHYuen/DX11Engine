#pragma once
#include <d3d11.h>
#include <directxmath.h>
using namespace DirectX;

class RenderTexture {
public:
    RenderTexture();
    RenderTexture(const RenderTexture&);
    ~RenderTexture();

    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, int textureWidth, int textureHeight, float nearZ, float farZ, DXGI_FORMAT textureFormat, float perspectiveFOV = XM_PIDIV4, int mipLevels = 1, int texArraySize = 1, bool b_IsCubeMap = false);
    void Shutdown();

    void SetRenderTarget();
    bool SetTextureArrayRenderTarget(ID3D11Device* device, int targetArrayIndex, int targetMipSlice, int targetWidth, int targetHeight, int arraySize = -1);
    void ClearRenderTarget(float red, float green, float blue, float alpha);

    ID3D11ShaderResourceView* GetTextureSRV() const { return m_ShaderResourceView; }
    void GetProjectionMatrix(XMMATRIX& projectionMatrix) const { projectionMatrix = m_ProjectionMatrix; }
    void GetOrthoMatrix(XMMATRIX& orthoMatrix) const { orthoMatrix = m_OrthoMatrix; }

    int GetTextureWidth() const { return m_TextureWidth; };
    int GetTextureHeight() const { return m_TextureHeight; };
    float GetNearZ() const { return m_NearZ; };
    float GetFarZ() const { return m_FarZ; };

    void EnableAlphaBlending();
    void DisableAlphaBlending();
    void EnableAdditiveBlending();

    void TurnZBufferOn();
    void TurnZBufferOff();

private:
    ID3D11DeviceContext* m_DeviceContext {};
    int m_TextureWidth {}, m_TextureHeight {};
    float m_NearZ {}, m_FarZ {};

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
    ID3D11BlendState* m_AdditiveBlendingState {};
};