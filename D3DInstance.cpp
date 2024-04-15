#include "D3DInstance.h"

bool D3DInstance::Initialize(int screenWidth, int screenHeight, bool vsync, HWND hwnd, bool b_IsFullscreen, float nearZ, float farZ) {
	HRESULT result {};

	m_Vsync_enabled = vsync;

	// Create a DirectX graphics interface factory.
	IDXGIFactory1* factory {};
	result = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory);
	if(FAILED(result)) {
		return false;
	}

	// Use the factory to create an adapter for the primary graphics interface (video card).
	IDXGIAdapter1* adapter {};
	result = factory->EnumAdapters1(0, &adapter);
	if(FAILED(result)) {
		return false;
	}

	// Enumerate the primary adapter output (monitor).
	IDXGIOutput* adapterOutput {};
	result = adapter->EnumOutputs(0, &adapterOutput);
	if(FAILED(result)) {
		return false;
	}

	// Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
	unsigned int numModes {};
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
	//result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
	if(FAILED(result)) {
		return false;
	}

	// Create a list to hold all the possible display modes for this monitor/video card combination.
	DXGI_MODE_DESC* displayModeList {};
	displayModeList = new DXGI_MODE_DESC[numModes];
	if(!displayModeList) {
		return false;
	}

	// Now fill the display mode list structures.
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList);
	//result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList);
	if(FAILED(result)) {
		return false;
	}

	// Now go through all the display modes and find the one that matches the screen width and height.
	// When a match is found store the numerator and denominator of the refresh rate for that monitor.
	for(unsigned int i = 0; i < numModes; i++) {
		if(displayModeList[i].Width == (unsigned int)screenWidth) {
			if(displayModeList[i].Height == (unsigned int)screenHeight) {
				m_RefreshRateNumerator = displayModeList[i].RefreshRate.Numerator;
				m_RefreshRateDenominator = displayModeList[i].RefreshRate.Denominator;
			}
		}
	}

	// Get the adapter (video card) description.
	DXGI_ADAPTER_DESC adapterDesc {};
	result = adapter->GetDesc(&adapterDesc);
	if(FAILED(result)) {
		return false;
	}

	// Store the dedicated video card memory in megabytes.
	m_VideoCardMemory = (int)(adapterDesc.DedicatedVideoMemory / 1024 / 1024);

	// Convert the name of the video card to a character array and store it.
	unsigned long long stringLength {};
	int error = wcstombs_s(&stringLength, m_VideoCardDescription, 128, adapterDesc.Description, 128);
	if(error != 0) {
		return false;
	}

	// Release the display mode list.
	delete[] displayModeList;
	displayModeList = nullptr;

	// Release the adapter output.
	adapterOutput->Release();
	adapterOutput = nullptr;

	// Release the adapter.
	adapter->Release();
	adapter = nullptr;

	// Release the factory.
	factory->Release();
	factory = nullptr;

	////////////////////////
	// START DIRECTX INIT //
	////////////////////////
	
	// Initialize the swap chain description.
	DXGI_SWAP_CHAIN_DESC swapChainDesc {};
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	// Must be 2-16 for flip model, 
	// see https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model 
	swapChainDesc.BufferCount = 2;

	// Set the width and height of the back buffer.
	swapChainDesc.BufferDesc.Width = screenWidth;
	swapChainDesc.BufferDesc.Height = screenHeight;

	//swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

	// Set the refresh rate of the back buffer.
	if(m_Vsync_enabled) {
		swapChainDesc.BufferDesc.RefreshRate.Numerator =   m_RefreshRateNumerator;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = m_RefreshRateDenominator;
	}
	else {
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	// Set the usage of the back buffer.
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Set the handle for the window to render to.
	swapChainDesc.OutputWindow = hwnd;

	// Turn multisampling off.
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// Set to full screen or windowed mode.
	swapChainDesc.Windowed = !b_IsFullscreen;

	// Set the scan line ordering and scaling to unspecified.
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Discard the back buffer contents after presenting.
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// Don't set the advanced flags.
	swapChainDesc.Flags = 0;

	// Set the feature level to DirectX 11.
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

	/// Create the swap chain, Direct3D device, and Direct3D device context.
	result = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
		//0,
		D3D11_CREATE_DEVICE_DEBUG,
		&featureLevel, 1,
		D3D11_SDK_VERSION, &swapChainDesc, &m_SwapChain, &m_Device, NULL, &m_DeviceContext);
	if(FAILED(result)) {
		return false;
	}

	// Get the pointer to the back buffer.
	ID3D11Texture2D* backBufferPtr {};
	result = m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferPtr);
	if(FAILED(result)) {
		return false;
	}

	// Create the render target view with the back buffer pointer.
	result = m_Device->CreateRenderTargetView(backBufferPtr, NULL, &m_RenderTargetView);
	if(FAILED(result)) {
		return false;
	}

	// Release pointer to the back buffer as we no longer need it.
	backBufferPtr->Release();
	backBufferPtr = nullptr;

	/// Create depth buffer
	D3D11_TEXTURE2D_DESC depthBufferDesc {};
	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));
	// Set up the description of the depth buffer.
	depthBufferDesc.Width = screenWidth;
	depthBufferDesc.Height = screenHeight;
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
	result = m_Device->CreateTexture2D(&depthBufferDesc, NULL, &m_DepthStencilBuffer);
	if(FAILED(result)) {
		return false;
	}

	/// Create default depth stencil
	// Initialize the description of the stencil state.
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc {};
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

	// Set up the description of the stencil state.
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;

	// Changed for cubemap optimization ("An optimization" https://learnopengl.com/Advanced-OpenGL/Cubemaps)
	//depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

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

	result = m_Device->CreateDepthStencilState(&depthStencilDesc, &m_DepthStencilState);
	if(FAILED(result)) {
		return false;
	}

	/// Set the default depth stencil state
	m_DeviceContext->OMSetDepthStencilState(m_DepthStencilState, 1);

	/// Create disabled depth state
	depthStencilDesc.DepthEnable = false;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	result = m_Device->CreateDepthStencilState(&depthStencilDesc, &m_DepthDisabledStencilState);
	if(FAILED(result)) {
		return false;
	}

	/// Initialize the depth stencil view.
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc {};
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));

	// Set up the depth stencil view description.
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	// Create the depth stencil view.
	result = m_Device->CreateDepthStencilView(m_DepthStencilBuffer, &depthStencilViewDesc, &m_DepthStencilView);
	if(FAILED(result)) {
		return false;
	}

	// Bind the render target view and depth stencil buffer to the output render pipeline.
	SetToBackBufferRenderTargetAndViewPort();

	/// Setup the raster description
	D3D11_RASTERIZER_DESC rasterDesc {};
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state from the description we just filled out.
	result = m_Device->CreateRasterizerState(&rasterDesc, &m_RasterStateBackCull);
	if(FAILED(result)) {
		return false;
	}

	rasterDesc.CullMode = D3D11_CULL_FRONT;
	result = m_Device->CreateRasterizerState(&rasterDesc, &m_RasterStateFrontCull);
	if(FAILED(result)) {
		return false;
	}

	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
	result = m_Device->CreateRasterizerState(&rasterDesc, &m_RasterStateWireBackCull);
	if(FAILED(result)) {
		return false;
	}

	// Now set the rasterizer state.
	m_DeviceContext->RSSetState(m_RasterStateBackCull);

	// Setup the viewport for rendering.
	m_Viewport.Width = (float)screenWidth;
	m_Viewport.Height = (float)screenHeight;
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;
	m_Viewport.TopLeftX = 0.0f;
	m_Viewport.TopLeftY = 0.0f;

	// Create the viewport.
	m_DeviceContext->RSSetViewports(1, &m_Viewport);

	// Setup the projection matrix.
	float screenAspect = (float)screenWidth / (float)screenHeight;

	// Create the projection matrix for 3D rendering.
	m_ProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(s_DefaultFOV, screenAspect, nearZ, farZ);

	// Initialize the world matrix to the identity matrix.
	m_WorldMatrix = DirectX::XMMatrixIdentity();

	// Create an orthographic projection matrix for 2D rendering.
	m_OrthoMatrix = DirectX::XMMatrixOrthographicLH((float)screenWidth, (float)screenHeight, nearZ, farZ);

	/// Create blend states
	// Enable Alpha Blend
	D3D11_BLEND_DESC blendStateDescription {};
	ZeroMemory(&blendStateDescription, sizeof(D3D11_BLEND_DESC));
	blendStateDescription.RenderTarget[0].BlendEnable = TRUE;
	blendStateDescription.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendStateDescription.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendStateDescription.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDescription.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendStateDescription.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDescription.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateDescription.RenderTarget[0].RenderTargetWriteMask = 0x0f;

	result = m_Device->CreateBlendState(&blendStateDescription, &m_EnableAlphaBlendingState);
	if(FAILED(result)) {
		return false;
	}

	// Enable Additive Blend
	blendStateDescription.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blendStateDescription.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	result = m_Device->CreateBlendState(&blendStateDescription, &m_EnableAdditiveBlendingState);
	if(FAILED(result)) {
		return false;
	}

	// Disable Blending
	blendStateDescription.RenderTarget[0].BlendEnable = FALSE;
	result = m_Device->CreateBlendState(&blendStateDescription, &m_DisableAlphaBlendingState);
	if(FAILED(result)) {
		return false;
	}

	return true;
}

bool D3DInstance::ResizeWindow(HWND hwnd, int newWidth, int newHeight, float screenNear, float screenFar) {
	/// Clear Swap chain refs
	m_DeviceContext->OMSetRenderTargets(0, 0, 0);
	m_RenderTargetView->Release();
	m_DepthStencilView->Release();
	m_DepthStencilBuffer->Release();

	/// Resize swap chain
	DXGI_MODE_DESC dxgiDesc {};
	dxgiDesc.Width = newWidth;
	dxgiDesc.Height = newHeight;
	if(m_Vsync_enabled) {
		dxgiDesc.RefreshRate.Numerator = m_RefreshRateNumerator;
		dxgiDesc.RefreshRate.Denominator = m_RefreshRateDenominator;
	}
	else {
		dxgiDesc.RefreshRate.Numerator = 0;
		dxgiDesc.RefreshRate.Denominator = 1;
	}
	dxgiDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	dxgiDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	HRESULT hr;
	hr = m_SwapChain->ResizeTarget(&dxgiDesc);
	if(FAILED(hr)) return false;

	SetWindowPos(hwnd, HWND_TOP, 0, 0, newWidth, newHeight, 0);

	// Note: "proper" way is to trigger the following with WM_SIZE in message loop
	hr = m_SwapChain->ResizeBuffers(2, newWidth, newHeight, DXGI_FORMAT_R16G16B16A16_FLOAT, 0);
	if(FAILED(hr)) return false;

	/// Recreate render target
	ID3D11Texture2D* backBufferPtr {};
	hr = m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferPtr);
	if(FAILED(hr)) return false;

	hr = m_Device->CreateRenderTargetView(backBufferPtr, NULL, &m_RenderTargetView);
	if(FAILED(hr)) return false;

	backBufferPtr->Release();
	backBufferPtr = nullptr;

	/// Recreate depth buffer/stencils
	D3D11_TEXTURE2D_DESC depthBufferDesc {};
	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));
	depthBufferDesc.Width = newWidth;
	depthBufferDesc.Height = newHeight;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;
	hr = m_Device->CreateTexture2D(&depthBufferDesc, NULL, &m_DepthStencilBuffer);
	if(FAILED(hr)) return false;

	// Initialize the depth stencil view.
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc {};
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;
	hr = m_Device->CreateDepthStencilView(m_DepthStencilBuffer, &depthStencilViewDesc, &m_DepthStencilView);
	if(FAILED(hr)) return false;

	/// Update viewport
	m_Viewport.Width = (float)newWidth;
	m_Viewport.Height = (float)newHeight;
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;
	m_Viewport.TopLeftX = 0.0f;
	m_Viewport.TopLeftY = 0.0f;

	/// Update rendering matrices
	float screenAspect = (float)newWidth / (float)newHeight;
	m_ProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(s_DefaultFOV, screenAspect, screenNear, screenFar);
	m_OrthoMatrix = DirectX::XMMatrixOrthographicLH((float)newWidth, (float)newHeight, screenNear, screenFar);

	return true;
}

void D3DInstance::ClearBackBuffer(float red, float green, float blue, float alpha) {
	float color[4];

	// Setup the color to clear the buffer to.
	color[0] = red;
	color[1] = green;
	color[2] = blue;
	color[3] = alpha;

	// Clear the back buffer.
	m_DeviceContext->ClearRenderTargetView(m_RenderTargetView, color);

	// Clear the depth buffer.
	m_DeviceContext->ClearDepthStencilView(m_DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void D3DInstance::SwapPresent() {
	// Present the back buffer to the screen since rendering is complete.
	if(m_Vsync_enabled) {
		// Lock to screen refresh rate.
		m_SwapChain->Present(1, 0);
	}
	else {
		// Present as fast as possible.
		m_SwapChain->Present(0, 0);
	}
}

void D3DInstance::SetToBackBufferRenderTargetAndViewPort() {
	m_DeviceContext->OMSetRenderTargets(1, &m_RenderTargetView, m_DepthStencilView);
	m_DeviceContext->RSSetViewports(1, &m_Viewport);
}

void D3DInstance::SetToWireBackCullRasterState() {
	m_DeviceContext->RSSetState(m_RasterStateWireBackCull);
}

void D3DInstance::SetToBackCullRasterState() {
	m_DeviceContext->RSSetState(m_RasterStateBackCull);
}

void D3DInstance::SetToFrontCullRasterState() {
	m_DeviceContext->RSSetState(m_RasterStateFrontCull);
}

void D3DInstance::TurnZBufferOn() {
	m_DeviceContext->OMSetDepthStencilState(m_DepthStencilState, 1);
}

void D3DInstance::TurnZBufferOff() {
	m_DeviceContext->OMSetDepthStencilState(m_DepthDisabledStencilState, 1);
}

void D3DInstance::EnableAlphaBlending() {
	m_DeviceContext->OMSetBlendState(m_EnableAlphaBlendingState, NULL, 0xffffffff);
}

void D3DInstance::DisableAlphaBlending() {
	m_DeviceContext->OMSetBlendState(m_DisableAlphaBlendingState, NULL, 0xffffffff);
}

void D3DInstance::EnableAdditiveBlending() {
	m_DeviceContext->OMSetBlendState(m_EnableAdditiveBlendingState, NULL, 0xffffffff);
}

void D3DInstance::Shutdown() {
	// Before shutting down set to windowed mode or when you release the swap chain it will throw an exception.
	if(m_SwapChain) {
		m_SwapChain->SetFullscreenState(false, NULL);
	}

	if(m_EnableAlphaBlendingState) {
		m_EnableAlphaBlendingState->Release();
		m_EnableAlphaBlendingState = nullptr;
	}

	if(m_DisableAlphaBlendingState) {
		m_DisableAlphaBlendingState->Release();
		m_DisableAlphaBlendingState = nullptr;
	}

	if(m_EnableAdditiveBlendingState) {
		m_EnableAdditiveBlendingState->Release();
		m_EnableAdditiveBlendingState = nullptr;
	}

	if(m_DepthDisabledStencilState) {
		m_DepthDisabledStencilState->Release();
		m_DepthDisabledStencilState = nullptr;
	}

	if(m_RasterStateBackCull) {
		m_RasterStateBackCull->Release();
		m_RasterStateBackCull = nullptr;
	}

	if(m_RasterStateFrontCull) {
		m_RasterStateFrontCull->Release();
		m_RasterStateFrontCull = nullptr;
	}

	if(m_DepthStencilView) {
		m_DepthStencilView->Release();
		m_DepthStencilView = nullptr;
	}

	if(m_DepthStencilState) {
		m_DepthStencilState->Release();
		m_DepthStencilState = nullptr;
	}

	if(m_DepthStencilBuffer) {
		m_DepthStencilBuffer->Release();
		m_DepthStencilBuffer = nullptr;
	}

	if(m_RenderTargetView) {
		m_RenderTargetView->Release();
		m_RenderTargetView = nullptr;
	}

	if(m_DeviceContext) {
		m_DeviceContext->Release();
		m_DeviceContext = nullptr;
	}

	if(m_Device) {
		m_Device->Release();
		m_Device = nullptr;
	}

	if(m_SwapChain) {
		m_SwapChain->Release();
		m_SwapChain = nullptr;
	}
}
