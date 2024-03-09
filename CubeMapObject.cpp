#include "CubeMapObject.h"
#include "ModelClass.h"
#include "TextureClass.h"
#include "RenderTextureClass.h"
#include "HDRTexture.h"
#include <fstream>

CubeMapObject::CubeMapObject() {}
CubeMapObject::CubeMapObject(const CubeMapObject& other) {}
CubeMapObject::~CubeMapObject() {}

// TODO: remove HDRTextureClass
bool CubeMapObject::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND hwnd, const std::string& fileName, int cubeFaceResolution) {
	/// Load shaders
	// Initialize the vertex and pixel shaders.
	bool result = InitializeShader(device, hwnd, L"HDRCubeMap", &m_HDREquiVertexShader, &m_HDREquiPixelShader);
	if(!result) {
		return false;
	}

	result = InitializeShader(device, hwnd, L"ConvoluteCubeMap", &m_ConvolutionVertexShader, &m_ConvolutionPixelShader);
	if(!result) {
		return false;
	}

	result = InitializeShader(device, hwnd, L"CubeMap", &m_CubeMapVertexShader, &m_CubeMapPixelShader);
	if(!result) {
		return false;
	}

	/// Load unit cube
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

	// Initialize 6 render textures for cubemap capture from equirectangularly mapped HDR texture
	std::array<RenderTextureClass*, 6> renderTextureArray {};
	for(size_t i = 0; i < 6; i++) {
		renderTextureArray[i] = new RenderTextureClass();
		result = renderTextureArray[i]->Initialize(device, deviceContext, cubeFaceResolution, cubeFaceResolution, 0.1f, 10.0f, DXGI_FORMAT_R32G32B32A32_FLOAT, XMConvertToRadians(90.0f));
		if(!result) {
			return false;
		}
	}

	// Same projection matrix for all captures (90 degree FOV)
	XMMATRIX cubemapCapturecaptureProjectionMatrix {};
	renderTextureArray[0]->GetProjectionMatrix(cubemapCapturecaptureProjectionMatrix);

	std::array<ID3D11Texture2D*, 6> sourceCubeMapTextures{};
	for(size_t i = 0; i < 6; i++) {
		renderTextureArray[i]->SetRenderTarget(deviceContext);
		// Red clear color for debugging
		renderTextureArray[i]->ClearRenderTarget(deviceContext, 0.5f, 0.0f, 0.0f, 1.0f);

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

	/// Render 6 textures with convolution shader and build irradiance map 
	// Initialize 6 render textures for convolution cubemap capture
	for(size_t i = 0; i < 6; i++) {
		renderTextureArray[i] = new RenderTextureClass();
		result = renderTextureArray[i]->Initialize(device, deviceContext, 32, 32, 0.1f, 10.0f, DXGI_FORMAT_R32G32B32A32_FLOAT, XMConvertToRadians(90.0f));
		if(!result) {
			return false;
		}
	}

	for(size_t i = 0; i < 6; i++) {
		renderTextureArray[i]->SetRenderTarget(deviceContext);
		// Red clear color for debugging
		renderTextureArray[i]->ClearRenderTarget(deviceContext, 0.5f, 0.0f, 0.0f, 1.0f);

		result = Render(deviceContext, kCubeMapCaptureViewMats[i], cubemapCapturecaptureProjectionMatrix, kConvolution);
		if(!result) {
			return false;
		}

		sourceCubeMapTextures[i] = renderTextureArray[i]->GetTexture();
	}

	// Create cubemap texture array (and SRV)
	m_IrradianceCubeMapTex = new TextureClass();
	result = m_IrradianceCubeMapTex->Initialize(device, deviceContext, sourceCubeMapTextures);
	if(!result) {
		return false;
	}

	/// Clean up local resources 
	for(size_t i = 0; i < 6; i++) {
		renderTextureArray[i]->Shutdown();
		delete renderTextureArray[i];
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

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	D3D11_BUFFER_DESC matrixBufferDesc {};
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;

	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	result = device->CreateBuffer(&matrixBufferDesc, NULL, &m_MatrixBuffer);
	if(FAILED(result)) {
		return false;
	}

	// Create a texture sampler state description.
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
	cubeMapSamplerDesc.MaxLOD = 0;
	//samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	// Create the texture sampler state.
	result = device->CreateSamplerState(&cubeMapSamplerDesc, &m_ClampSampleState);
	if(FAILED(result)) {
		return false;
	}

	return true;
}


bool CubeMapObject::Render(ID3D11DeviceContext* deviceContext, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, RenderType renderType) {
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

	// Write to matrix constant buffer
	D3D11_MAPPED_SUBRESOURCE mappedResource {};
	HRESULT result = deviceContext->Map(m_MatrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if(FAILED(result)) {
		return false;
	}

	MatrixBufferType* dataPtr = (MatrixBufferType*)mappedResource.pData;

	dataPtr->view = viewMatrix;
	dataPtr->projection = projectionMatrix;

	deviceContext->Unmap(m_MatrixBuffer, 0);
	unsigned int bufferNumber = 0;
	deviceContext->VSSetConstantBuffers(bufferNumber, 1, &m_MatrixBuffer);

	// Set shader texture resource in the pixel shader.
	ID3D11ShaderResourceView* cubeMapTexture {};

	/// Render cubemap shader on unit cube
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
		case kSkyBox:
			cubeMapTexture = m_CubeMapTex->GetTextureSRV();
			// DEBUG
			//cubeMapTexture = m_IrradianceCubeMapTex->GetTextureSRV();
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
	// Release the sampler state.
	if(m_ClampSampleState) {
		m_ClampSampleState->Release();
		m_ClampSampleState = nullptr;
	}

	// Release the matrix constant buffer.
	if(m_MatrixBuffer) {
		m_MatrixBuffer->Release();
		m_MatrixBuffer = nullptr;
	}

	// Release the layout.
	if(m_Layout) {
		m_Layout->Release();
		m_Layout = nullptr;
	}

	// Release the pixel shader.
	if(m_CubeMapPixelShader) {
		m_CubeMapPixelShader->Release();
		m_CubeMapPixelShader = nullptr;
	}

	// Release the vertex shader.
	if(m_CubeMapVertexShader) {
		m_CubeMapVertexShader->Release();
		m_CubeMapVertexShader = nullptr;
	}

	if(m_HDRCubeMapTex) {
		m_HDRCubeMapTex->Shutdown();
		m_HDRCubeMapTex = nullptr;
	}

	if(m_CubeMapTex) {
		m_CubeMapTex->Shutdown();
		m_CubeMapTex = nullptr;
	}
}

