#include "CubeMapObject.h"
#include "QuadModel.h"
#include "RenderTexture.h"
#include "Texture.h"
#include "D3DInstance.h"
#include <fstream>

static constexpr int kUnitCubeVertexCount = 36;
static constexpr int kUnitCubeIndexCount = 36;
static constexpr int kUnitQuadIndexCount = 6;
// x, y, z, u, v
static constexpr float kUnitCubeVertices[] = {
	-1.0 , 1.0, -1.0, 0.0 , 0.0,
	 1.0 , 1.0, -1.0, 1.0 , 0.0,
	-1.0, -1.0, -1.0, 0.0,  1.0,
	-1.0, -1.0, -1.0, 0.0,  1.0,
	 1.0 , 1.0, -1.0, 1.0 , 0.0,
	 1.0 ,-1.0, -1.0, 1.0 , 1.0,
	 1.0 , 1.0, -1.0, 0.0 , 0.0,
	 1.0 , 1.0,  1.0, 1.0 , 0.0,
	 1.0 ,-1.0, -1.0, 0.0 , 1.0,
	 1.0 ,-1.0, -1.0, 0.0 , 1.0,
	 1.0 , 1.0,  1.0, 1.0 , 0.0,
	 1.0 ,-1.0,  1.0, 1.0 , 1.0,
	 1.0 , 1.0,  1.0, 0.0 , 0.0,
	-1.0,  1.0,  1.0, 1.0,  0.0,
	 1.0 ,-1.0,  1.0, 0.0 , 1.0,
	 1.0 ,-1.0,  1.0, 0.0 , 1.0,
	-1.0,  1.0,  1.0, 1.0,  0.0,
	-1.0, -1.0,  1.0, 1.0,  1.0,
	-1.0,  1.0,  1.0, 0.0,  0.0,
	-1.0,  1.0, -1.0, 1.0,  0.0,
	-1.0, -1.0,  1.0, 0.0,  1.0,
	-1.0, -1.0,  1.0, 0.0,  1.0,
	-1.0,  1.0, -1.0, 1.0,  0.0,
	-1.0, -1.0, -1.0, 1.0,  1.0,
	-1.0,  1.0,  1.0, 0.0,  0.0,
	 1.0 , 1.0,  1.0, 1.0 , 0.0,
	-1.0,  1.0, -1.0, 0.0,  1.0,
	-1.0,  1.0, -1.0, 0.0,  1.0,
	 1.0 , 1.0,  1.0, 1.0 , 0.0,
	 1.0 , 1.0, -1.0, 1.0 , 1.0,
	-1.0, -1.0, -1.0, 0.0,  0.0,
	 1.0 ,-1.0, -1.0, 1.0 , 0.0,
	-1.0, -1.0,  1.0, 0.0,  1.0,
	-1.0, -1.0,  1.0, 0.0,  1.0,
	 1.0 ,-1.0, -1.0, 1.0 , 0.0,
	 1.0 ,-1.0,  1.0, 1.0 , 1.0,
};

// View matrices for the 6 different cube directions
static constexpr XMFLOAT3 float3_000  {  0.0f,   0.0f,  0.0f };
static constexpr XMFLOAT3 float3_100  {  1.0f,   0.0f,  0.0f };
static constexpr XMFLOAT3 float3_010  {  0.0f,   1.0f,  0.0f };
static constexpr XMFLOAT3 float3_n100 { -1.0f,   0.0f,  0.0f };
static constexpr XMFLOAT3 float3_00n1 {  0.0f,   0.0f, -1.0f };
static constexpr XMFLOAT3 float3_0n10 {  0.0f,  -1.0f,  0.0f };
static constexpr XMFLOAT3 float3_001  {  0.0f,   0.0f,  1.0f };
static const std::array<XMMATRIX, 6> kCubeMapCaptureViewMats = {
	XMMatrixLookAtLH(XMLoadFloat3(&float3_000), XMLoadFloat3(&float3_100),  XMLoadFloat3(&float3_010)),
	XMMatrixLookAtLH(XMLoadFloat3(&float3_000), XMLoadFloat3(&float3_n100), XMLoadFloat3(&float3_010)),
	XMMatrixLookAtLH(XMLoadFloat3(&float3_000), XMLoadFloat3(&float3_010),  XMLoadFloat3(&float3_00n1)),
	XMMatrixLookAtLH(XMLoadFloat3(&float3_000), XMLoadFloat3(&float3_0n10),	XMLoadFloat3(&float3_001)),
	XMMatrixLookAtLH(XMLoadFloat3(&float3_000), XMLoadFloat3(&float3_001),	XMLoadFloat3(&float3_010)),
	XMMatrixLookAtLH(XMLoadFloat3(&float3_000), XMLoadFloat3(&float3_00n1), XMLoadFloat3(&float3_010)),
};

static const std::wstring kHDRCubeMapShaderName       = L"HDRCubeMap";
static const std::wstring kConvoluteCubeMapShaderName = L"ConvoluteCubeMap";
static const std::wstring kPrefilterCubeMapShaderName = L"PreFilterCubeMap";
static const std::wstring kIntegrateBRDFShaderName    = L"IntegrateBRDF";
static const std::wstring kSkyboxRenderShaderName     = L"CubeMap";

bool CubeMapObject::Initialize(D3DInstance* d3dInstance, HWND hwnd, const std::string& fileName, int cubeFaceResolution, int cubeMapMipLevels, int irradianceMapResolution, int fullPrefilterMapResolution, int precomputedBRDFResolution, XMMATRIX screenDisplayViewMatrix, XMMATRIX screenOrthoMatrix, QuadModel* screenDisplayQuad) {
	bool result;

	ID3D11Device* device = d3dInstance->GetDevice();
	ID3D11DeviceContext* deviceContext = d3dInstance->GetDeviceContext();
	
	/// Create texture sampler states
	D3D11_SAMPLER_DESC clampSamplerDesc{};
	clampSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	clampSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	clampSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	clampSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	clampSamplerDesc.MipLODBias = 0.0f;
	clampSamplerDesc.MaxAnisotropy = 1;
	clampSamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	clampSamplerDesc.BorderColor[0] = 0;
	clampSamplerDesc.BorderColor[1] = 0;
	clampSamplerDesc.BorderColor[2] = 0;
	clampSamplerDesc.BorderColor[3] = 0;
	clampSamplerDesc.MinLOD = 0;
	clampSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	HRESULT hresult = device->CreateSamplerState(&clampSamplerDesc, &m_ClampSampleState);
	if(FAILED(hresult)) {
		return false;
	}

	/// Initialize the vertex and pixel shaders
	/// Note: only needs to be done once, change this if loading more than one cubemap
	result = InitializeShader(device, hwnd, kHDRCubeMapShaderName, &m_HDREquiVertexShader, &m_HDREquiPixelShader);
	if(!result) return false; 

	result = InitializeShader(device, hwnd, kConvoluteCubeMapShaderName, &m_ConvolutionVertexShader, &m_ConvolutionPixelShader);
	if(!result) return false;

	result = InitializeShader(device, hwnd, kPrefilterCubeMapShaderName, &m_PrefilterVertexShader, &m_PrefilterPixelShader);
	if(!result) return false;

	result = InitializeShader(device, hwnd, kIntegrateBRDFShaderName, &m_IntegrateBRDFVertexShader, &m_IntegrateBRDFPixelShader);
	if(!result) return false;

	result = InitializeShader(device, hwnd, kSkyboxRenderShaderName, &m_CubeMapVertexShader, &m_CubeMapPixelShader);
	if(!result) return false;

	/// Load unit cube model
	result = InitializeUnitCubeBuffers(device);
	if(!result) return false;


	/// Load HDR cubemap texture from disk and render to 6 cubemap textures to build skybox
	d3dInstance->SetToFrontCullRasterState();

	// NOTE: HDRTexture defaults to no mipmaps
	m_HDRCubeMapTex = new Texture();
	result = m_HDRCubeMapTex->Initialize(device, deviceContext, "../DX11Engine/data/" + fileName + ".hdr", DXGI_FORMAT_R32G32B32A32_FLOAT, 1);
	if(!result) {
		return false;
	}

	/// Load HDR cubemap texture from disk and render to 6 cubemap textures to build skybox
	m_CubeMapTex = new RenderTexture();
	m_CubeMapTex->Initialize(device, deviceContext, cubeFaceResolution, cubeFaceResolution, 0.1f, 10.0f,
		DXGI_FORMAT_R32G32B32A32_FLOAT, XMConvertToRadians(90.0f),
		cubeMapMipLevels, /* use MipLevels (for prefilter step)*/
		6, true /*IsCubeMap*/
	);

	// Same projection matrix for all cubemap captures (90 degree FOV)
	XMMATRIX cubemapCapturecaptureProjectionMatrix{};
	m_CubeMapTex->GetProjectionMatrix(cubemapCapturecaptureProjectionMatrix);
	
	// Render HDR texture to 6 cubemap textures using equirectangular coords
	for(int i = 0; i < 6; i++) {
		result = m_CubeMapTex->SetTextureArrayRenderTarget(device, i, 0, cubeFaceResolution, cubeFaceResolution, 1);
		if(!result) return false;
		m_CubeMapTex->ClearRenderTarget(0.5f, 0.0f, 0.0f, 1.0f);

		result = Render(deviceContext, kCubeMapCaptureViewMats[i], cubemapCapturecaptureProjectionMatrix, kHDRCaptureRender);
		if(!result) return false;
	}
	
	// Generate mipmaps for completed skybox (for prefilter step)
	deviceContext->GenerateMips(m_CubeMapTex->GetTextureSRV());

	/// Capture 6 textures with convolution shader and build irradiance cubemap (diffuse IBL)
	m_IrradianceCubeMapTex = new RenderTexture();
	m_IrradianceCubeMapTex->Initialize(device, deviceContext, irradianceMapResolution, irradianceMapResolution, 0.1f, 10.0f,
		DXGI_FORMAT_R32G32B32A32_FLOAT, XMConvertToRadians(90.0f),
		1 /* MipLevels (Don't use mips for irradiance map)*/,
		6, true /*IsCubeMap*/
	);

	// Capture 6 textures with convolution shader and build irradiance cubemap (diffuse IBL)
	for(int i = 0; i < 6; i++) {
		result = m_IrradianceCubeMapTex->SetTextureArrayRenderTarget(device, i, 0, irradianceMapResolution, irradianceMapResolution, 1);
		if(!result) return false;
		m_IrradianceCubeMapTex->ClearRenderTarget(0.5f, 0.0f, 0.0f, 1.0f);

		result = Render(deviceContext, kCubeMapCaptureViewMats[i], cubemapCapturecaptureProjectionMatrix, kIrradianceConvolutionRender);
		if(!result) return false;
	}

	/// Render 6 textures with prefilter shader (with roughness dependent mipmaps) and build prefiltered environment map (speclular IBL)
	m_PrefilteredCubeMapTex = new RenderTexture();
	m_PrefilteredCubeMapTex->Initialize(device, deviceContext, fullPrefilterMapResolution, fullPrefilterMapResolution, 0.1f, 10.0f, DXGI_FORMAT_R32G32B32A32_FLOAT, XMConvertToRadians(90.0f), cubeMapMipLevels, 6, true /*isCubeMap*/);
	if(!result) {
		return false;
	}

	// Capture 6 cubemap directions with mips for prefiltered map
	for(int mipSlice = 0; mipSlice < cubeMapMipLevels; mipSlice++) {
		int currMipSize = (int)(fullPrefilterMapResolution * std::pow(0.5, mipSlice));
		for(int i = 0; i < 6; i++) {
			result = m_PrefilteredCubeMapTex->SetTextureArrayRenderTarget(device, i, mipSlice, currMipSize, currMipSize);
			if(!result) return false;
			m_PrefilteredCubeMapTex->ClearRenderTarget(0.5f, 0.0f, 0.0f, 1.0f);

			float roughness = (float)mipSlice / (float)(cubeMapMipLevels - 1);
			result = Render(deviceContext, kCubeMapCaptureViewMats[i], cubemapCapturecaptureProjectionMatrix, kPrefilterRender, roughness);
			if(!result) return false;
		}
	}

	d3dInstance->SetToBackCullRasterState();
	
	/// Precompute BRDF (independent of environment maps, can be stored outside of class instance)
	m_PrecomputedBRDFTex = new RenderTexture();
	m_PrecomputedBRDFTex->Initialize(device, deviceContext, precomputedBRDFResolution, precomputedBRDFResolution, 0.1f, 10.0f, DXGI_FORMAT_R16G16_FLOAT);
	m_PrecomputedBRDFTex->SetRenderTarget();
	m_PrecomputedBRDFTex->ClearRenderTarget(0.0f, 0.0f, 0.0f, 1.0f);
	screenDisplayQuad->Render(deviceContext);
	result = Render(deviceContext, screenDisplayViewMatrix, screenOrthoMatrix, kIntegrateBRDFRender);
	if(!result) return false;

	return true;
}

bool CubeMapObject::InitializeUnitCubeBuffers(ID3D11Device* device) {
	VertexType* vertices = new VertexType[kUnitCubeVertexCount];
	unsigned long* indices = new unsigned long[kUnitCubeIndexCount];

	// Load the vertex array and index array with hard coded unit cube data
	for(int i = 0, j = 0; i < kUnitCubeVertexCount; i++, j += 5) {
		vertices[i].position = XMFLOAT3(kUnitCubeVertices[j], kUnitCubeVertices[j + 1], kUnitCubeVertices[j + 2]);
		vertices[i].uv = XMFLOAT2(kUnitCubeVertices[j + 3], kUnitCubeVertices[j + 4]);
		indices[i] = i;
	}

	// Set up the description of the static vertex buffer.
	D3D11_BUFFER_DESC vertexBufferDesc {};
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * kUnitCubeVertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	D3D11_SUBRESOURCE_DATA vertexData {};
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Now create the vertex buffer.
	HRESULT result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_VertexBuffer);
	if(FAILED(result)) {
		return false;
	}

	// Set up the description of the static index buffer.
	D3D11_BUFFER_DESC indexBufferDesc {};
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * kUnitCubeIndexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the index data.
	D3D11_SUBRESOURCE_DATA indexData {};
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer.
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_IndexBuffer);
	if(FAILED(result)) {
		return false;
	}

	// Release the arrays now that the vertex and index buffers have been created and loaded.
	delete[] vertices;
	vertices = nullptr;

	delete[] indices;
	indices = nullptr;

	return true;
}

bool CubeMapObject::InitializeShader(ID3D11Device* device, HWND hwnd, std::wstring shaderName, ID3D11VertexShader** ppVertShader, ID3D11PixelShader** ppPixelShader) {
	HRESULT result {};
	ID3D10Blob* errorMessage {};
	ID3D10Blob* vertexShaderBuffer {};
	ID3D10Blob* pixelShaderBuffer {};

	std::wstring vsFileName = L"../DX11Engine/Shaders/" + shaderName + L".vs";
	std::wstring psFileName = L"../DX11Engine/Shaders/" + shaderName + L".ps";

	// Compile the vertex shader code.
	result = D3DCompileFromFile(vsFileName.c_str(), NULL, NULL, "Vert", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
		&vertexShaderBuffer, &errorMessage);
	if(FAILED(result)) {
		// If the shader failed to compile it should have writen something to the error message.
		if(errorMessage) {
			OutputShaderErrorMessage(errorMessage, hwnd, (WCHAR*)vsFileName.c_str());
		}
		// If there was  nothing in the error message then it simply could not find the shader file itself.
		else {
			MessageBox(hwnd, vsFileName.c_str(), L"Missing Shader File", MB_OK);
		}

		return false;
	}

	// Compile the pixel shader code.
	result = D3DCompileFromFile(psFileName.c_str(), NULL, NULL, "Frag", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
		&pixelShaderBuffer, &errorMessage);
	if(FAILED(result)) {
		// If the shader failed to compile it should have writen something to the error message.
		if(errorMessage) {
			OutputShaderErrorMessage(errorMessage, hwnd, (WCHAR*)psFileName.c_str());
		}
		// If there was nothing in the error message then it simply could not find the file itself.
		else {
			MessageBox(hwnd, psFileName.c_str(), L"Missing Shader File", MB_OK);
		}

		return false;
	}

	// Create the vertex shader from the buffer.
	result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, ppVertShader);
	if(FAILED(result)) {
		return false;
	}

	// Create the pixel shader from the buffer.
	result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, ppPixelShader);
	if(FAILED(result)) {
		return false;
	}

	// Create the vertex input layout description.
	// This setup needs to match the VertexType stucture in the ModelClass and in the shader.
	D3D11_INPUT_ELEMENT_DESC polygonLayout[2] {};
	polygonLayout[0].SemanticName = "POSITION";
	polygonLayout[0].SemanticIndex = 0;
	polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[0].InputSlot = 0;
	polygonLayout[0].AlignedByteOffset = 0;
	polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[0].InstanceDataStepRate = 0;

	polygonLayout[1].SemanticName = "TEXCOORD";
	polygonLayout[1].SemanticIndex = 0;
	polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	polygonLayout[1].InputSlot = 0;
	polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[1].InstanceDataStepRate = 0;

	// Get a count of the elements in the layout.
	unsigned int numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

	// Create the vertex input layout.
	result = device->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(),
		vertexShaderBuffer->GetBufferSize(), &m_Layout);
	if(FAILED(result)) {
		return false;
	}

	// Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
	vertexShaderBuffer->Release();
	vertexShaderBuffer = nullptr;

	pixelShaderBuffer->Release();
	pixelShaderBuffer = nullptr;

	/// Setup the description of dynamic matrix cbuffer in vertex shader
	D3D11_BUFFER_DESC matrixBufferDesc{};
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;

	result = device->CreateBuffer(&matrixBufferDesc, NULL, &m_MatrixBuffer);
	if(FAILED(result)) {
		return false;
	}

	/// Set up description of prefilter cbuffer in frag shader (for roughness param)
	if(shaderName == kPrefilterCubeMapShaderName) {
		D3D11_BUFFER_DESC prefilterBufferDesc{};
		prefilterBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		prefilterBufferDesc.ByteWidth = sizeof(PrefilterBufferType);
		prefilterBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		prefilterBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		prefilterBufferDesc.MiscFlags = 0;
		prefilterBufferDesc.StructureByteStride = 0;

		result = device->CreateBuffer(&prefilterBufferDesc, NULL, &m_PrefilterParamBuffer);
		if(FAILED(result)) {
			return false;
		}
	}

	return true;
}

bool CubeMapObject::Render(ID3D11DeviceContext* deviceContext, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, RenderType renderType, float roughness) {
	if(renderType != kIntegrateBRDFRender) {
		/// Render Unit Cube
		unsigned int stride = sizeof(VertexType);
		unsigned int offset = 0;
		deviceContext->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
		deviceContext->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// removing translation in view matrix by truncating 4x4 matrix to 3x3
		XMFLOAT3X3 viewMatrix3x3{};
		XMStoreFloat3x3(&viewMatrix3x3, viewMatrix);
		viewMatrix = XMLoadFloat3x3(&viewMatrix3x3);

		viewMatrix = DirectX::XMMatrixTranspose(viewMatrix);
		projectionMatrix = DirectX::XMMatrixTranspose(projectionMatrix);
	}

	/// Write to matrix constant buffer
	D3D11_MAPPED_SUBRESOURCE mappedResource{};
	HRESULT result = deviceContext->Map(m_MatrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if(FAILED(result)) {
		return false;
	}

	MatrixBufferType* dataPtr = (MatrixBufferType*)mappedResource.pData;
	dataPtr->view = viewMatrix;
	dataPtr->projection = projectionMatrix;

	deviceContext->Unmap(m_MatrixBuffer, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &m_MatrixBuffer);

	/// Write to roughness cbuffer if this is cubemap prefilter render
	if(renderType == kPrefilterRender) {
		HRESULT result = deviceContext->Map(m_PrefilterParamBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		if(FAILED(result)) {
			return false;
		}

		PrefilterBufferType* dataPtr = (PrefilterBufferType*)mappedResource.pData;
		dataPtr->roughness = roughness;

		deviceContext->Unmap(m_PrefilterParamBuffer, 0);
		deviceContext->PSSetConstantBuffers(0, 1, &m_PrefilterParamBuffer);
	}
	

	/// Render cubemap shader on unit cube
	ID3D11ShaderResourceView* cubeMapTexture {};
	switch(renderType) {
		case kHDRCaptureRender:
			cubeMapTexture = m_HDRCubeMapTex->GetTextureSRV();
			deviceContext->VSSetShader(m_HDREquiVertexShader, NULL, 0);
			deviceContext->PSSetShader(m_HDREquiPixelShader, NULL, 0);
			break;
		case kIrradianceConvolutionRender:
			cubeMapTexture = m_CubeMapTex->GetTextureSRV();
			deviceContext->VSSetShader(m_ConvolutionVertexShader, NULL, 0);
			deviceContext->PSSetShader(m_ConvolutionPixelShader, NULL, 0);
			break;
		case kPrefilterRender:
			cubeMapTexture = m_CubeMapTex->GetTextureSRV();
			deviceContext->VSSetShader(m_PrefilterVertexShader, NULL, 0);
			deviceContext->PSSetShader(m_PrefilterPixelShader, NULL, 0);
			break;
		case kIntegrateBRDFRender:
			deviceContext->VSSetShader(m_IntegrateBRDFVertexShader, NULL, 0);
			deviceContext->PSSetShader(m_IntegrateBRDFPixelShader, NULL, 0);
			break;
		case kSkyBoxRender:
			cubeMapTexture = m_CubeMapTex->GetTextureSRV();
			// DEBUG
			//cubeMapTexture = m_IrradianceCubeMapTex->GetTextureSRV();
			//cubeMapTexture = m_PrefilteredCubeMapTex->GetTextureSRV();
			deviceContext->VSSetShader(m_CubeMapVertexShader, NULL, 0);
			deviceContext->PSSetShader(m_CubeMapPixelShader, NULL, 0);
			break;
		default:
			return false;
	}

	deviceContext->IASetInputLayout(m_Layout);
	deviceContext->PSSetSamplers(0, 1, &m_ClampSampleState);

	if(renderType == kIntegrateBRDFRender) {
		deviceContext->DrawIndexed(kUnitQuadIndexCount, 0, 0);
	}
	else {
		deviceContext->PSSetShaderResources(0, 1, &cubeMapTexture);
		deviceContext->DrawIndexed(kUnitCubeIndexCount, 0, 0);
	}

	return true;
}

ID3D11ShaderResourceView* CubeMapObject::GetIrradianceMapSRV() const { return m_IrradianceCubeMapTex->GetTextureSRV(); }
ID3D11ShaderResourceView* CubeMapObject::GetPrefilteredMapSRV() const { return m_PrefilteredCubeMapTex->GetTextureSRV(); }
ID3D11ShaderResourceView* CubeMapObject::GetPrecomputedBRDFSRV() const { return m_PrecomputedBRDFTex->GetTextureSRV(); }

void CubeMapObject::OutputShaderErrorMessage(ID3D10Blob* errorMessage, HWND hwnd, WCHAR* shaderFilename) {
	char* compileErrors;
	unsigned long long bufferSize, i;
	std::ofstream fout;

	// Get a pointer to the error message text buffer.
	compileErrors = (char*)(errorMessage->GetBufferPointer());

	// Get the length of the message.
	bufferSize = errorMessage->GetBufferSize();

	// Open a file to write the error message to.
	fout.open("shader-error.txt");

	// Write out the error message.
	for(i = 0; i < bufferSize; i++) {
		fout << compileErrors[i];
	}

	// Close the file.
	fout.close();

	// Release the error message.
	errorMessage->Release();
	errorMessage = nullptr;

	// Pop a message up on the screen to notify the user to check the text file for compile errors.
	MessageBox(hwnd, L"Error compiling shader.  Check shader-error.txt for message.", shaderFilename, MB_OK);

	return;
}

// Shutdown the vertex and pixel shaders as well as the related objects.
void CubeMapObject::Shutdown() {
	if(m_ClampSampleState) {
		m_ClampSampleState->Release();
		m_ClampSampleState = nullptr;
	}

	if(m_MatrixBuffer) {
		m_MatrixBuffer->Release();
		m_MatrixBuffer = nullptr;
	}

	if(m_PrefilterParamBuffer) {
		m_PrefilterParamBuffer->Release();
		m_PrefilterParamBuffer = nullptr;
	}

	if(m_Layout) {
		m_Layout->Release();
		m_Layout = nullptr;
	}

	/// Shaders
	if(m_CubeMapPixelShader) {
		m_CubeMapPixelShader->Release();
		m_CubeMapPixelShader = nullptr;
	}

	if(m_CubeMapVertexShader) {
		m_CubeMapVertexShader->Release();
		m_CubeMapVertexShader = nullptr;
	}

	if(m_HDREquiPixelShader) {
		m_HDREquiPixelShader->Release();
		m_HDREquiPixelShader = nullptr;
	}

	if(m_HDREquiVertexShader) {
		m_HDREquiVertexShader->Release();
		m_HDREquiVertexShader = nullptr;
	}

	if(m_ConvolutionPixelShader) {
		m_ConvolutionPixelShader->Release();
		m_ConvolutionPixelShader = nullptr;
	}

	if(m_ConvolutionVertexShader) {
		m_ConvolutionVertexShader->Release();
		m_ConvolutionVertexShader = nullptr;
	}

	if(m_PrefilterPixelShader) {
		m_PrefilterPixelShader->Release();
		m_PrefilterPixelShader = nullptr;
	}

	if(m_PrefilterVertexShader) {
		m_PrefilterVertexShader->Release();
		m_PrefilterVertexShader = nullptr;
	}

	/// Textures
	if(m_HDRCubeMapTex) {
		m_HDRCubeMapTex->Shutdown();
		m_HDRCubeMapTex = nullptr;
	}

	if(m_CubeMapTex) {
		m_CubeMapTex->Shutdown();
		m_CubeMapTex = nullptr;
	}

	if(m_IrradianceCubeMapTex) {
		m_IrradianceCubeMapTex->Shutdown();
		m_IrradianceCubeMapTex = nullptr;
	}

	if(m_PrefilteredCubeMapTex) {
		m_PrefilteredCubeMapTex->Shutdown();
		m_PrefilteredCubeMapTex = nullptr;
	}
}

