#pragma once

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <d3d11.h>
#include <directxmath.h>
using namespace DirectX;

class D3DClass {
public:
    D3DClass();
    D3DClass(const D3DClass&);
    ~D3DClass();

    bool Initialize(int, int, bool, HWND, bool, float, float);
    void Shutdown();

    void BeginScene(float, float, float, float);
    void EndScene();

    ID3D11Device* GetDevice();
    ID3D11DeviceContext* GetDeviceContext();

    void GetProjectionMatrix(DirectX::XMMATRIX&);
    void GetWorldMatrix(DirectX::XMMATRIX&);
    void GetOrthoMatrix(DirectX::XMMATRIX&);

    void GetVideoCardInfo(char*, int&);

    void SetToBackBufferRenderTarget();
    void ResetViewport();

    void SetToBackCullRasterState();
    void SetToFrontCullRasterState();

    void TurnZBufferOn();
    void TurnZBufferOff();

    void EnableAlphaBlending();
    void DisableAlphaBlending();

private:
    bool m_vsync_enabled {};
    int m_videoCardMemory {};
    char m_videoCardDescription[128] {};
    IDXGISwapChain* m_swapChain {};
    ID3D11Device* m_device {};
    ID3D11DeviceContext* m_deviceContext {};
    ID3D11RenderTargetView* m_renderTargetView {};
    
    ID3D11Texture2D* m_depthStencilBuffer {};
    ID3D11DepthStencilView* m_depthStencilView {};
    ID3D11DepthStencilState* m_depthStencilState {};
    ID3D11DepthStencilState* m_depthDisabledStencilState {};

    ID3D11RasterizerState* m_rasterStateBackCull {};
    ID3D11RasterizerState* m_rasterStateFrontCull {};
    DirectX::XMMATRIX m_projectionMatrix {};
    DirectX::XMMATRIX m_worldMatrix {};
    DirectX::XMMATRIX m_orthoMatrix {};
    D3D11_VIEWPORT m_viewport {};

    ID3D11BlendState* m_alphaEnableBlendingState {};
    ID3D11BlendState* m_alphaDisableBlendingState {};
};
