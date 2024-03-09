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
    bool m_Vsync_enabled {};
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

    ID3D11RasterizerState* m_RasterStateBackCull {};
    ID3D11RasterizerState* m_RasterStateFrontCull {};
    DirectX::XMMATRIX m_ProjectionMatrix {};
    DirectX::XMMATRIX m_WorldMatrix {};
    DirectX::XMMATRIX m_OrthoMatrix {};
    D3D11_VIEWPORT m_Viewport {};

    ID3D11BlendState* m_AlphaEnableBlendingState {};
    ID3D11BlendState* m_AlphaDisableBlendingState {};
};
