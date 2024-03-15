#include "CubeMapObject.h"
#include "ModelClass.h"
#include "TextureClass.h"
#include "RenderTextureClass.h"
#include "HDRTexture.h"
#include <fstream>

static constexpr int kUnitCubeVertexCount = 36;
static constexpr int kUnitCubeIndexCount = 36;
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
static const std::wstring kSkyboxRenderShaderName     = L"CubeMap";

CubeMapObject::CubeMapObject() {}
CubeMapObject::CubeMapObject(const CubeMapObject& other) {}
CubeMapObject::~CubeMapObject() {}

// TODO: REMOVE HDRTexture
bool CubeMapObject::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND hwnd, const std::string& fileName, int cubeFaceResolution) {
	/// Initialize the vertex and pixel shaders
	bool result = InitializeShader(device, hwnd, kHDRCubeMapShaderName, &m_HDREquiVertexShader, &m_HDREquiPixelShader);
	if(!result) {
		return false;
	}

	result = InitializeShader(device, hwnd, kConvoluteCubeMapShaderName, &m_ConvolutionVertexShader, &m_ConvolutionPixelShader);
	if(!result) {
		return false;
	}

	result = InitializeShader(device, hwnd, kPrefilterCubeMapShaderName, &m_PrefilterVertexShader, &m_PrefilterPixelShader);
	if(!result) {
		return false;
	}

	result = InitializeShader(device, hwnd, kSkyboxRenderShaderName, &m_CubeMapVertexShader, &m_CubeMapPixelShader);
	if(!result) {
		return false;
	}

	/// Load unit cube model
	result = InitializeUnitCubeBuffers(device);
	if(!result) {
		return false;
	}

	/// Load HDR cubemap texture from disk and render to 6 cubemap textures to build skybox
	m_HDRCubeMapTex = new HDRTexture();
	result = m_HDRCubeMapTex->Initialize(device, deviceContext, "../DX11Engine/data/" + fileName + ".hdr");
	if(!result) {
		return false;
	}

	constexpr int kMaxPrefilterMipLevels = 9;

	// Initialize 6 render textures for cubemap capture from equirectangularly mapped HDR texture
	std::array<RenderTextureClass*, 6> renderTextureArray {};
	for(size_t i = 0; i < 6; i++) {
		renderTextureArray[i] = new RenderTextureClass();
		result = renderTextureArray[i]->Initialize(
			device, deviceContext, cubeFaceResolution, cubeFaceResolution, 0.1f, 10.0f,
			DXGI_FORMAT_R32G32B32A32_FLOAT, XMConvertToRadians(90.0f),
			kMaxPrefilterMipLevels /* MipLevels (for prefilter step)*/
		);
		if(!result) {
			return false;
		}
	}

	// TODO: render directly into "sourceCubeMapTextures" with render target views (use D3D11_RTV_DIMENSION_TEXTURE2DARRAY and Texture2DArray member in desc)

	// Same projection matrix for all captures (90 degree FOV)
	XMMATRIX cubemapCapturecaptureProjectionMatrix {};
	renderTextureArray[0]->GetProjectionMatrix(cubemapCapturecaptureProjectionMatrix);

	std::array<ID3D11Texture2D*, 6> sourceCubeMapTextures{};
	for(size_t i = 0; i < 6; i++) {
		renderTextureArray[i]->SetRenderTarget();
		// Red clear color for debugging
		renderTextureArray[i]->ClearRenderTarget(0.5f, 0.0f, 0.0f, 1.0f);

		result = Render(deviceContext, kCubeMapCaptureViewMats[i], cubemapCapturecaptureProjectionMatrix, kHDRCapture);
		if(!result) {
			return false;
		}

		sourceCubeMapTextures[i] = renderTextureArray[i]->GetTexture();
	}

	// Create cubemap texture array (and SRV)
	m_CubeMapTex = new TextureClass();
	result = m_CubeMapTex->Initialize(device, deviceContext, sourceCubeMapTextures);
	if(!result) {
		return false;
	}

	/// Capture 6 textures with convolution shader and build irradiance cibemap (diffuse IBL)
	for(size_t i = 0; i < 6; i++) {
		renderTextureArray[i] = new RenderTextureClass();
		result = renderTextureArray[i]->Initialize(device, deviceContext, 32, 32, 0.1f, 10.0f, DXGI_FORMAT_R32G32B32A32_FLOAT, XMConvertToRadians(90.0f));
		if(!result) {
			return false;
		}

		renderTextureArray[i]->SetRenderTarget();
		// Red clear color for debugging
		renderTextureArray[i]->ClearRenderTarget(0.5f, 0.0f, 0.0f, 1.0f);

		result = Render(deviceContext, kCubeMapCaptureViewMats[i], cubemapCapturecaptureProjectionMatrix, kConvolution);
		if(!result) {
			return false;
		}

		sourceCubeMapTextures[i] = renderTextureArray[i]->GetTexture();
	}

	// Create irradiance cubemap SRV
	m_IrradianceCubeMapTex = new TextureClass();
	result = m_IrradianceCubeMapTex->Initialize(device, deviceContext, sourceCubeMapTextures);
	if(!result) {
		return false;
	}

	/// Render 6 textures with prefilter shader (with roughness dependent mipmaps) and build prefiltered environment map (speclular IBL)
	constexpr int kMaxPrefilterResolution = 512;

	m_PrefilteredCubeMapTex = new RenderTextureClass();
	m_PrefilteredCubeMapTex->Initialize(device, deviceContext, kMaxPrefilterResolution, kMaxPrefilterResolution, 0.1f, 10.0f, DXGI_FORMAT_R32G32B32A32_FLOAT, XMConvertToRadians(90.0f), kMaxPrefilterMipLevels, 6, true /*isCubeMap*/);
	if(!result) {
		return false;
	}

	// Capture 6 cubemap directions
	for(int mipSlice = 0; mipSlice < kMaxPrefilterMipLevels; mipSlice++) {
		int currMipSize = kMaxPrefilterResolution * std::pow(0.5, mipSlice);
		for(int i = 0; i < 6; i++) {
			result = m_PrefilteredCubeMapTex->SetTextureArrayRenderTarget(device, i, 6, mipSlice, currMipSize, currMipSize);
			if(!result) {
				return false;
			}

			m_PrefilteredCubeMapTex->ClearRenderTarget(0.5f, 0.0f, 0.0f, 1.0f);

			float roughness = (float)mipSlice / (float)(kMaxPrefilterMipLevels - 1);
			result = Render(deviceContext, kCubeMapCaptureViewMats[i], cubemapCapturecaptureProjectionMatrix, kPrefilter, roughness);
			if(!result) {
				return false;
			}
		}
	}

	/// Clean up local resources 
	for(size_t i = 0; i < 6; i++) {
		renderTextureArray[i]->Shutdown();
		delete renderTextureArray[i];
		renderTextureArray[i] = nullptr;
		sourceCubeMapTextures[i]->Release();
	}

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

	std::wstring vsFileName = L"../DX11Engine/" + shaderName + L".vs";
	std::wstring psFileName = L"../DX11Engine/" + shaderName + L".ps";

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
	D3D11_BUFFER_DESC matrixBufferDesc {};
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

	/// Create a texture sampler state description.
	D3D11_SAMPLER_DESC cubeMapSamplerDesc {};
	cubeMapSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	cubeMapSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	cubeMapSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	cubeMapSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	cubeMapSamplerDesc.MipLODBias = 0.0f;
	cubeMapSamplerDesc.MaxAnisotropy = 1;
	cubeMapSamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	cubeMapSamplerDesc.BorderColor[0] = 0;
	cubeMapSamplerDesc.BorderColor[1] = 0;
	cubeMapSamplerDesc.BorderColor[2] = 0;
	cubeMapSamplerDesc.BorderColor[3] = 0;

	// LODs disabled for now, cubemap mipmaps not figured out yet
	cubeMapSamplerDesc.MinLOD = 0;
	//cubeMapSamplerDesc.MaxLOD = 0;
	cubeMapSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	// Create the texture sampler state.
	result = device->CreateSamplerState(&cubeMapSamplerDesc, &m_ClampSampleState);
	if(FAILED(result)) {
		return false;
	}

	return true;
}


bool CubeMapObject::Render(ID3D11DeviceContext* deviceContext, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, RenderType renderType, float roughness) {
	/// Render Unit Cube
	unsigned int stride = sizeof(VertexType);
	unsigned int offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
	deviceContext->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	/// Update cubemap shader resources
	// removing translation in view matrix by truncating 4x4 matrix to 3x3
	XMFLOAT3X3 viewMatrix3x3{};
	XMStoreFloat3x3(&viewMatrix3x3, viewMatrix);
	viewMatrix = XMLoadFloat3x3(&viewMatrix3x3);

	viewMatrix = DirectX::XMMatrixTranspose(viewMatrix);
	projectionMatrix = DirectX::XMMatrixTranspose(projectionMatrix);

	/// Write to matrix constant buffer
	D3D11_MAPPED_SUBRESOURCE mappedResource {};
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
	if(renderType == kPrefilter) {
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
		case kHDRCapture:
			cubeMapTexture = m_HDRCubeMapTex->GetTextureSRV();
			deviceContext->VSSetShader(m_HDREquiVertexShader, NULL, 0);
			deviceContext->PSSetShader(m_HDREquiPixelShader, NULL, 0);
			break;
		case kConvolution:
			cubeMapTexture = m_CubeMapTex->GetTextureSRV();
			deviceContext->VSSetShader(m_ConvolutionVertexShader, NULL, 0);
			deviceContext->PSSetShader(m_ConvolutionPixelShader, NULL, 0);
			break;
		case kPrefilter:
			cubeMapTexture = m_CubeMapTex->GetTextureSRV();
			deviceContext->VSSetShader(m_PrefilterVertexShader, NULL, 0);
			deviceContext->PSSetShader(m_PrefilterPixelShader, NULL, 0);
			break;
		case kSkyBox:
			//cubeMapTexture = m_CubeMapTex->GetTextureSRV();
			// DEBUG
			//cubeMapTexture = m_IrradianceCubeMapTex->GetTextureSRV();
			cubeMapTexture = m_PrefilteredCubeMapTex->GetTextureSRV();
			deviceContext->VSSetShader(m_CubeMapVertexShader, NULL, 0);
			deviceContext->PSSetShader(m_CubeMapPixelShader, NULL, 0);
			break;
		default:
			return false;
	}

	deviceContext->PSSetShaderResources(0, 1, &cubeMapTexture);
	deviceContext->IASetInputLayout(m_Layout);
	deviceContext->PSSetSamplers(0, 1, &m_ClampSampleState);

	deviceContext->DrawIndexed(kUnitCubeIndexCount, 0, 0);

	return true;
}

ID3D11ShaderResourceView* CubeMapObject::GetIrradianceSRV() const { 
	return m_IrradianceCubeMapTex->GetTextureSRV(); 
}

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

	if(m_PrefilteredCubeMapTex) {
		m_PrefilteredCubeMapTex->Shutdown();
		m_PrefilteredCubeMapTex = nullptr;
	}
}

