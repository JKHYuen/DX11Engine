#include "RenderTextureClass.h"

RenderTextureClass::RenderTextureClass() {
    m_RenderTargetTexture = nullptr;
    m_RenderTargetView    = nullptr;
    m_ShaderResourceView  = nullptr;
    m_DepthStencilBuffer  = nullptr;
    m_DepthStencilView    = nullptr;
}

RenderTextureClass::RenderTextureClass(const RenderTextureClass& other) {}
RenderTextureClass::~RenderTextureClass() {}

bool RenderTextureClass::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, int textureWidth, int textureHeight, float nearZ, float farZ, DXGI_FORMAT textureFormat, float perspectiveFOV) {
    D3D11_TEXTURE2D_DESC textureDesc {};
    HRESULT result {};
    D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc {};
    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc {};
    D3D11_TEXTURE2D_DESC depthBufferDesc {};
    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc {};

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc {};
    D3D11_DEPTH_STENCIL_DESC depthDisabledStencilDesc {};
    D3D11_BLEND_DESC blendStateDescription {};

    m_DeviceContext = deviceContext;

    // Store the width and height of the render texture.
    m_TextureWidth = textureWidth;
    m_TextureHeight = textureHeight;

    // Initialize the render target texture description.
    ZeroMemory(&textureDesc, sizeof(textureDesc));

    // Setup the render target texture description.
    textureDesc.Width = textureWidth;
    textureDesc.Height = textureHeight;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = textureFormat;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    // Create the render target texture.
    result = device->CreateTexture2D(&textureDesc, NULL, &m_RenderTargetTexture);
    if(FAILED(result)) {
        return false;
    }

    // Setup the description of the render target view.
    renderTargetViewDesc.Format = textureDesc.Format;
    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    renderTargetViewDesc.Texture2D.MipSlice = 0;

    // Create the render target view.
    result = device->CreateRenderTargetView(m_RenderTargetTexture, &renderTargetViewDesc, &m_RenderTargetView);
    if(FAILED(result)) {
        return false;
    }

    // Setup the description of the shader resource view.
    shaderResourceViewDesc.Format = textureDesc.Format;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;

    // Create the shader resource view.
    result = device->CreateShaderResourceView(m_RenderTargetTexture, &shaderResourceViewDesc, &m_ShaderResourceView);
    if(FAILED(result)) {
        return false;
    }

    // Initialize the description of the depth buffer.
    ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

    // Set up the description of the depth buffer.
    depthBufferDesc.Width = textureWidth;
    depthBufferDesc.Height = textureHeight;
    depthBufferDesc.MipLevels = 1;
    depthBufferDesc.ArraySize = 1;
    depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthBufferDesc.SampleDesc.Count = 1;
    depthBufferDesc.SampleDesc.Quality = 0;
    depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthBufferDesc.CPUAccessFlags = 0;
    depthBufferDesc.MiscFlags = 0;

    // Create the texture for the depth buffer using the filled out description.
    result = device->CreateTexture2D(&depthBufferDesc, NULL, &m_DepthStencilBuffer);
    if(FAILED(result)) {
        return false;
    }

    // Initailze the depth stencil view description.
    ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));

    // Set up the depth stencil view description.
    depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Texture2D.MipSlice = 0;

    // Create the depth stencil view.
    result = device->CreateDepthStencilView(m_DepthStencilBuffer, &depthStencilViewDesc, &m_DepthStencilView);
    if(FAILED(result)) {
        return false;
    }

    /// BLEND STATE
    // Clear the blend state description.
    ZeroMemory(&blendStateDescription, sizeof(D3D11_BLEND_DESC));

    // Create an alpha enabled blend state description.
    blendStateDescription.RenderTarget[0].BlendEnable = TRUE;
    blendStateDescription.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendStateDescription.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendStateDescription.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendStateDescription.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendStateDescription.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendStateDescription.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendStateDescription.RenderTarget[0].RenderTargetWriteMask = 0x0f;

    // Create the blend state using the description.
    result = device->CreateBlendState(&blendStateDescription, &m_AlphaEnableBlendingState);
    if(FAILED(result)) {
        return false;
    }

    // Modify the description to create an alpha disabled blend state description.
    blendStateDescription.RenderTarget[0].BlendEnable = FALSE;
    // Create the blend state using the description.
    result = device->CreateBlendState(&blendStateDescription, &m_AlphaDisableBlendingState);
    if(FAILED(result)) {
        return false;
    }
    ///

    /// DEPTH STENCILS
    // Initialize the description of the stencil state.
    ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

    // Enabled depth stencil
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthStencilDesc.StencilEnable = true;
    depthStencilDesc.StencilReadMask = 0xFF;
    depthStencilDesc.StencilWriteMask = 0xFF;
    depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // Create the depth stencil state.
    result = device->CreateDepthStencilState(&depthStencilDesc, &m_DepthStencilState);
    if(FAILED(result)) {
        return false;
    }

    // Disabled depth stencil
    depthStencilDesc.DepthEnable = false;
    result = device->CreateDepthStencilState(&depthStencilDesc, &m_DepthDisabledStencilState);
    if(FAILED(result)) {
        return false;
    }

    // Set the default depth stencil state.
    m_DeviceContext->OMSetDepthStencilState(m_DepthStencilState, 1);
    ///

    // Setup the viewport for rendering.
    m_Viewport.Width = (float)textureWidth;
    m_Viewport.Height = (float)textureHeight;
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;
    m_Viewport.TopLeftX = 0;
    m_Viewport.TopLeftY = 0;

    // Setup the projection matrix.
    m_ProjectionMatrix = XMMatrixPerspectiveFovLH(perspectiveFOV, ((float)textureWidth / (float)textureHeight), nearZ, farZ);

    // Create an orthographic projection matrix for 2D rendering.
    m_OrthoMatrix = XMMatrixOrthographicLH((float)textureWidth, (float)textureHeight, nearZ, farZ);

    return true;
}

void RenderTextureClass::SetRenderTarget(ID3D11DeviceContext* deviceContext) {
    // Bind the render target view and depth stencil buffer to the output render pipeline.
    deviceContext->OMSetRenderTargets(1, &m_RenderTargetView, m_DepthStencilView);

    // Set the viewport.
    deviceContext->RSSetViewports(1, &m_Viewport);
}

void RenderTextureClass::ClearRenderTarget(ID3D11DeviceContext* deviceContext, float red, float green, float blue, float alpha) {
    float color[4];

    // Setup the color to clear the buffer to.
    color[0] = red;
    color[1] = green;
    color[2] = blue;
    color[3] = alpha;

    // Clear the back buffer.
    deviceContext->ClearRenderTargetView(m_RenderTargetView, color);

    // Clear the depth buffer.
    deviceContext->ClearDepthStencilView(m_DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

ID3D11ShaderResourceView* RenderTextureClass::GetShaderResourceView() {
    return m_ShaderResourceView;
}

void RenderTextureClass::GetProjectionMatrix(XMMATRIX& projectionMatrix) {
    projectionMatrix = m_ProjectionMatrix;
}

void RenderTextureClass::GetOrthoMatrix(XMMATRIX& orthoMatrix) {
    orthoMatrix = m_OrthoMatrix;
}

void RenderTextureClass::TurnZBufferOn() {
    m_DeviceContext->OMSetDepthStencilState(m_DepthStencilState, 1);
}

void RenderTextureClass::TurnZBufferOff() {
    m_DeviceContext->OMSetDepthStencilState(m_DepthDisabledStencilState, 1);
}

void RenderTextureClass::EnableAlphaBlending() {
    float blendFactor[4];

    // Setup the blend factor.
    blendFactor[0] = 0.0f;
    blendFactor[1] = 0.0f;
    blendFactor[2] = 0.0f;
    blendFactor[3] = 0.0f;

    // Turn on the alpha blending.
    m_DeviceContext->OMSetBlendState(m_AlphaEnableBlendingState, blendFactor, 0xffffffff);
}

void RenderTextureClass::DisableAlphaBlending() {
    float blendFactor[4];

    // Setup the blend factor.
    blendFactor[0] = 0.0f;
    blendFactor[1] = 0.0f;
    blendFactor[2] = 0.0f;
    blendFactor[3] = 0.0f;

    // Turn off the alpha blending.
    m_DeviceContext->OMSetBlendState(m_AlphaDisableBlendingState, blendFactor, 0xffffffff);
}

void RenderTextureClass::Shutdown() {
    if(m_DepthStencilView) {
        m_DepthStencilView->Release();
        m_DepthStencilView = nullptr;
    }

    if(m_DepthStencilBuffer) {
        m_DepthStencilBuffer->Release();
        m_DepthStencilBuffer = nullptr;
    }

    if(m_ShaderResourceView) {
        m_ShaderResourceView->Release();
        m_ShaderResourceView = nullptr;
    }

    if(m_RenderTargetView) {
        m_RenderTargetView->Release();
        m_RenderTargetView = nullptr;
    }

    if(m_RenderTargetTexture) {
        m_RenderTargetTexture->Release();
        m_RenderTargetTexture = nullptr;
    }
}
