#include "CubeMapObject.h"
#include "ModelClass.h"
#include "TextureClass.h"
#include "HDRTexture.h"
#include <fstream>

CubeMapObject::CubeMapObject() {}
CubeMapObject::CubeMapObject(const CubeMapObject& other) {}
CubeMapObject::~CubeMapObject() {}

bool CubeMapObject::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND hwnd, const std::string& textureFolderName) {
	/// Load shader
	bool result;
	wchar_t vsFilename[128];
	wchar_t psFilename[128];
	int error;

	// Set the filename of the vertex shader.
	//error = wcscpy_s(vsFilename, 128, L"../DX11Engine/CubeMap.vs");
	error = wcscpy_s(vsFilename, 128, L"../DX11Engine/HDRCubeMap.vs");
	if(error != 0) {
		return false;
	}

	// Set the filename of the pixel shader.
	//error = wcscpy_s(psFilename, 128, L"../DX11Engine/CubeMap.ps");
	error = wcscpy_s(psFilename, 128, L"../DX11Engine/HDRCubeMap.ps");
	if(error != 0) {
		return false;
	}

	// Initialize the vertex and pixel shaders.
	result = InitializeShader(device, hwnd, vsFilename, psFilename);
	if(!result) {
		return false;
	}
	///

	/// Load unit cube
	result = InitializeUnitCubeBuffers(device);
	if(!result) {
		return false;
	}

	//m_CubeMapTex = new TextureClass();
	//result = m_CubeMapTex->Initialize(device, deviceContext, "../DX11Engine/data/" + textureFolderName, /* isCubemap */ true);
	//if(!result) {
	//	return false;
	//}

	m_HDRCubeMapTex = new HDRTexture();
	result = m_HDRCubeMapTex->Initialize(device, deviceContext, "../DX11Engine/data/kloppenheim_03_4k.hdr");
	if(!result) {
		return false;
	}

	return true;
}

bool CubeMapObject::InitializeUnitCubeBuffers(ID3D11Device* device) {
	VertexType* vertices = new VertexType[k_UnitCubeVertexCount];
	unsigned long* indices = new unsigned long[k_UnitCubeIndexCount];

	// Load the vertex array and index array with hard coded unit cube data
	for(int i = 0, j = 0; i < k_UnitCubeVertexCount; i++, j += 5) {
		vertices[i].position = XMFLOAT3(k_UnitCubeVertices[j], k_UnitCubeVertices[j + 1], k_UnitCubeVertices[j + 2]);
		vertices[i].uv = XMFLOAT2(k_UnitCubeVertices[j + 3], k_UnitCubeVertices[j + 4]);
		indices[i] = i;
	}

	// Set up the description of the static vertex buffer.
	D3D11_BUFFER_DESC vertexBufferDesc {};
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * k_UnitCubeVertexCount;
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
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * k_UnitCubeIndexCount;
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

bool CubeMapObject::InitializeShader(ID3D11Device* device, HWND hwnd, WCHAR* vsFilename, WCHAR* psFilename) {
	HRESULT result {};
	ID3D10Blob* errorMessage {};
	ID3D10Blob* vertexShaderBuffer {};
	ID3D10Blob* pixelShaderBuffer {};

	// Compile the vertex shader code.
	result = D3DCompileFromFile(vsFilename, NULL, NULL, "Vert", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
		&vertexShaderBuffer, &errorMessage);
	if(FAILED(result)) {
		// If the shader failed to compile it should have writen something to the error message.
		if(errorMessage) {
			OutputShaderErrorMessage(errorMessage, hwnd, vsFilename);
		}
		// If there was  nothing in the error message then it simply could not find the shader file itself.
		else {
			MessageBox(hwnd, vsFilename, L"Missing Shader File", MB_OK);
		}

		return false;
	}

	// Compile the pixel shader code.
	result = D3DCompileFromFile(psFilename, NULL, NULL, "Frag", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
		&pixelShaderBuffer, &errorMessage);
	if(FAILED(result)) {
		// If the shader failed to compile it should have writen something to the error message.
		if(errorMessage) {
			OutputShaderErrorMessage(errorMessage, hwnd, psFilename);
		}
		// If there was nothing in the error message then it simply could not find the file itself.
		else {
			MessageBox(hwnd, psFilename, L"Missing Shader File", MB_OK);
		}

		return false;
	}

	// Create the vertex shader from the buffer.
	result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &m_VertexShader);
	if(FAILED(result)) {
		return false;
	}

	// Create the pixel shader from the buffer.
	result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &m_PixelShader);
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
	result = device->CreateSamplerState(&cubeMapSamplerDesc, &m_SampleState);
	if(FAILED(result)) {
		return false;
	}

	return true;
}


bool CubeMapObject::Render(ID3D11DeviceContext* deviceContext, XMMATRIX viewMatrix, XMMATRIX projectionMatrix) {
	/// Render Unit Cube
	unsigned int stride = sizeof(VertexType);
	unsigned int offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
	deviceContext->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	/// Update cubemap shader resources
	// removing translation in view matrix by truncating 4x4 matrix to 3x3
	XMFLOAT3X3 viewMatrix3x3 {};
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
	//ID3D11ShaderResourceView* cubeMapTexture = m_CubeMapTex->GetTextureSRV();
	ID3D11ShaderResourceView* cubeMapTexture = m_HDRCubeMapTex->GetTextureSRV();
	deviceContext->PSSetShaderResources(0, 1, &cubeMapTexture);

	/// Render cubemap shader on unit cube
	deviceContext->IASetInputLayout(m_Layout);

	deviceContext->VSSetShader(m_VertexShader, NULL, 0);
	deviceContext->PSSetShader(m_PixelShader, NULL, 0);
	deviceContext->PSSetSamplers(0, 1, &m_SampleState);

	deviceContext->DrawIndexed(k_UnitCubeIndexCount, 0, 0);

	return true;
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
	if(m_SampleState) {
		m_SampleState->Release();
		m_SampleState = nullptr;
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
	if(m_PixelShader) {
		m_PixelShader->Release();
		m_PixelShader = nullptr;
	}

	// Release the vertex shader.
	if(m_VertexShader) {
		m_VertexShader->Release();
		m_VertexShader = nullptr;
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

