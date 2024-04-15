#pragma once

#include <d3d11.h>
#include <directxmath.h>
using namespace DirectX;

class D3DInstance {
public:
    D3DInstance() {}
    D3DInstance(const D3DInstance&) {}
    ~D3DInstance() {}

    bool Initialize(int screenWidth, int screenHeight, bool vsync, HWND hwnd, bool b_IsFullscreen, float nearZ, float farZ);
    void Shutdown();

    bool ResizeWindow(HWND hwnd, int newWidth, int newHeight, float screenNear, float screenFar);
    void ClearBackBuffer(float red, float green, float blue, float alpha);
    void SwapPresent();

    ID3D11Device* GetDevice() const { return m_Device; }
    ID3D11DeviceContext* GetDeviceContext() const { return m_DeviceContext; }

    void GetProjectionMatrix(DirectX::XMMATRIX& projectionMatrix) const { projectionMatrix = m_ProjectionMatrix; }
    void GetWorldMatrix(DirectX::XMMATRIX& worldMatrix) const { worldMatrix = m_WorldMatrix; }
    void GetOrthoMatrix(DirectX::XMMATRIX& orthoMatrix) const { orthoMatrix = m_OrthoMatrix; }
    void GetVideoCardInfo(char* cardName, int& memory) const {
        strcpy_s(cardName, 128, m_VideoCardDescription);
        memory = m_VideoCardMemory;
    }

    void SetToBackBufferRenderTargetAndViewPort();

    void SetToWireBackCullRasterState();
    void SetToBackCullRasterState();
    void SetToFrontCullRasterState();

    void TurnZBufferOn();
    void TurnZBufferOff();

    void EnableAlphaBlending();
    void DisableAlphaBlending();
    void EnableAdditiveBlending();

private:
    static constexpr inline float s_DefaultFOV = DirectX::XM_PIDIV4;
    bool m_Vsync_enabled {};
    
    // Note: refresh rate detected on app launch, not updated at any point afterwards
    unsigned int m_RefreshRateNumerator {};
    unsigned int m_RefreshRateDenominator {};

    int m_VideoCardMemory {};
    char m_VideoCardDescription[128] {};

    IDXGISwapChain* m_SwapChain {};
    ID3D11Device* m_Device {};
    ID3D11DeviceContext* m_DeviceContext {};
    ID3D11RenderTargetView* m_RenderTargetView {};
    
    ID3D11Texture2D* m_DepthStencilBuffer {};
    ID3D11DepthStencilView* m_DepthStencilView {};
    ID3D11DepthStencilState* m_DepthStencilState {};
    ID3D11DepthStencilState* m_DepthDisabledStencilState {};

    ID3D11RasterizerState* m_RasterStateWireBackCull {};
    ID3D11RasterizerState* m_RasterStateBackCull {};
    ID3D11RasterizerState* m_RasterStateFrontCull {};
    DirectX::XMMATRIX m_ProjectionMatrix {};
    DirectX::XMMATRIX m_WorldMatrix {};
    DirectX::XMMATRIX m_OrthoMatrix {};
    D3D11_VIEWPORT m_Viewport {};

    ID3D11BlendState* m_EnableAlphaBlendingState {};
    ID3D11BlendState* m_DisableAlphaBlendingState {};
    ID3D11BlendState* m_EnableAdditiveBlendingState {};
};
