#include "RenderTextureClass.h"

RenderTextureClass::RenderTextureClass() {}
RenderTextureClass::RenderTextureClass(const RenderTextureClass& other) {}
RenderTextureClass::~RenderTextureClass() {}

bool RenderTextureClass::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, int textureWidth, int textureHeight, float nearZ, float farZ, DXGI_FORMAT textureFormat, float perspectiveFOV, int mipLevels, int texArraySize, bool b_IsCubeMap) {
    HRESULT result {};

    // force proper cubemap array size
    if(b_IsCubeMap) {
        texArraySize = 6;
    }

    m_DeviceContext = deviceContext;

    bool b_GenerateMips = mipLevels == 0 || mipLevels > 1;

    // Store the width and height of the render texture.
    m_TextureWidth = textureWidth;
    m_TextureHeight = textureHeight;

    // Setup the render target texture description.
    D3D11_TEXTURE2D_DESC textureDesc {};
    ZeroMemory(&textureDesc, sizeof(textureDesc));
    textureDesc.Width = textureWidth;
    textureDesc.Height = textureHeight;
    textureDesc.ArraySize = texArraySize;
    textureDesc.MipLevels = mipLevels;
    textureDesc.Format = textureFormat;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    if(b_IsCubeMap) {
        textureDesc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
    }

    if(b_GenerateMips) {
        textureDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
    }

    // Create the render target texture.
    result = device->CreateTexture2D(&textureDesc, NULL, &m_RenderTargetTexture);
    if(FAILED(result)) {
        return false;
    }

    // Setup the description of the render target view.
    D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc {};
    renderTargetViewDesc.Format = textureDesc.Format;
    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    renderTargetViewDesc.Texture2D.MipSlice = 0;

    // Create the render target view.
    result = device->CreateRenderTargetView(m_RenderTargetTexture, &renderTargetViewDesc, &m_RenderTargetView);
    if(FAILED(result)) {
        return false;
    }

    // Setup the description of the shader resource view.
    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc {};
    shaderResourceViewDesc.Format = textureDesc.Format;

    if(texArraySize == 1) {
        shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
        shaderResourceViewDesc.Texture2D.MipLevels = mipLevels;
    }
    else if(b_IsCubeMap) {
        shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        shaderResourceViewDesc.TextureCube.MostDetailedMip = 0;
        shaderResourceViewDesc.TextureCube.MipLevels = mipLevels;
    }
    else {
        shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        shaderResourceViewDesc.Texture2DArray.MostDetailedMip = 0;
        shaderResourceViewDesc.Texture2DArray.MipLevels = mipLevels;
        shaderResourceViewDesc.Texture2DArray.FirstArraySlice = 0;
        shaderResourceViewDesc.Texture2DArray.ArraySize = texArraySize;
    }

    // Create the shader resource view.
    result = device->CreateShaderResourceView(m_RenderTargetTexture, &shaderResourceViewDesc, &m_ShaderResourceView);
    if(FAILED(result)) {
        return false;
    }

    /// DEPTH BUFFER
    D3D11_TEXTURE2D_DESC depthBufferDesc{};
    ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));
    depthBufferDesc.Width = textureWidth;
    depthBufferDesc.Height = textureHeight;
    depthBufferDesc.MipLevels = mipLevels;
    depthBufferDesc.ArraySize = texArraySize;
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

    // Set up the depth stencil view description.
    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
    ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));

    if(texArraySize == 1) {
        depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        depthStencilViewDesc.Texture2D.MipSlice = 0;
    }
    else {
        depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        depthStencilViewDesc.Texture2DArray.MipSlice = 0;
        depthStencilViewDesc.Texture2DArray.FirstArraySlice = 0;
        depthStencilViewDesc.Texture2DArray.ArraySize = texArraySize;
    }

    // Create the depth stencil view.
    result = device->CreateDepthStencilView(m_DepthStencilBuffer, &depthStencilViewDesc, &m_DepthStencilView);
    if(FAILED(result)) {
        return false;
    }

    /// DEPTH STENCILS
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc{};
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

    /// BLEND STATE
    // Create an alpha enabled blend state description.
    D3D11_BLEND_DESC blendStateDescription{};
    ZeroMemory(&blendStateDescription, sizeof(D3D11_BLEND_DESC));

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

    /// Setup the viewport for rendering.
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

void RenderTextureClass::SetRenderTarget() {
    m_DeviceContext->OMSetRenderTargets(1, &m_RenderTargetView, m_DepthStencilView);
    m_DeviceContext->RSSetViewports(1, &m_Viewport);
}

bool RenderTextureClass::SetTextureArrayRenderTarget(ID3D11Device* device, int targetArrayIndex, int targetMipSlice, int targetWidth, int targetHeight, int arraySize) {

    D3D11_TEXTURE2D_DESC texElementDesc;
    m_RenderTargetTexture->GetDesc(&texElementDesc);

    D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc {};
    renderTargetViewDesc.Format = texElementDesc.Format;
    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
    renderTargetViewDesc.Texture2DArray.MipSlice = targetMipSlice;
    renderTargetViewDesc.Texture2DArray.FirstArraySlice = targetArrayIndex;
    renderTargetViewDesc.Texture2DArray.ArraySize = arraySize;

    // Create new render target view pointing to target array index and mip level
    HRESULT result = device->CreateRenderTargetView(m_RenderTargetTexture, &renderTargetViewDesc, &m_RenderTargetView);
    if(FAILED(result)) {
        return false;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc {};
    depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
    depthStencilViewDesc.Texture2DArray.MipSlice = targetMipSlice;
    depthStencilViewDesc.Texture2DArray.FirstArraySlice = targetArrayIndex;
    depthStencilViewDesc.Texture2DArray.ArraySize = arraySize; 

    // Create the new depth stencil view.
    result = device->CreateDepthStencilView(m_DepthStencilBuffer, &depthStencilViewDesc, &m_DepthStencilView);
    if(FAILED(result)) {
        return false;
    }

    // Don't forget this
    m_Viewport.Width = (float)targetWidth;
    m_Viewport.Height = (float)targetHeight;

    // TODO:
    //m_ProjectionMatrix = XMMatrixPerspectiveFovLH(perspectiveFOV, ((float)targetWidth / (float)targetHeight), nearZ, farZ);
    //m_OrthoMatrix = XMMatrixOrthographicLH((float)targetWidth, (float)targetHeight, nearZ, farZ);

    SetRenderTarget();

    return true;
}

void RenderTextureClass::ClearRenderTarget(float red, float green, float blue, float alpha) {
    float color[4];

    // Setup the color to clear the buffer to.
    color[0] = red;
    color[1] = green;
    color[2] = blue;
    color[3] = alpha;

    // Clear the back buffer
    m_DeviceContext->ClearRenderTargetView(m_RenderTargetView, color);

    // Clear the depth buffer
    m_DeviceContext->ClearDepthStencilView(m_DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

ID3D11ShaderResourceView* RenderTextureClass::GetTextureSRV() {
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