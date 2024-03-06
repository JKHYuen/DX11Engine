#include "CubeMapObject.h"
#include "ModelClass.h"
#include "TextureClass.h"
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
	error = wcscpy_s(vsFilename, 128, L"../DX11Engine/CubeMap.vs");
	if(error != 0) {
		return false;
	}

	// Set the filename of the pixel shader.
	error = wcscpy_s(psFilename, 128, L"../DX11Engine/CubeMap.ps");
	if(error != 0) {
		return false;
	}

	// Initialize the vertex and pixel shaders.
	result = InitializeShader(device, hwnd, vsFilename, psFilename);
	if(!result) {
		return false;
	}
	///

	// Load cubemap texture and cube model
	m_UnitCubeModel = new ModelClass();
	result = m_UnitCubeModel->Initialize(device, deviceContext,
		"../DX11Engine/data/cube.txt",
		{"../DX11Engine/data/" + textureFolderName},
		true // isCubeMap
	);
	if(!result) {
		return false;
	}

	return true;
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

	if(m_UnitCubeModel) {
		m_UnitCubeModel->Shutdown();
		m_UnitCubeModel = nullptr;
	}

	//if(m_hdrTexture) {
	//	m_hdrTexture->Shutdown();
	//	m_hdrTexture = nullptr;
	//}

	if(m_CubeMapTex) {
		m_CubeMapTex->Shutdown();
		m_CubeMapTex = nullptr;
	}
}

bool CubeMapObject::Render(ID3D11DeviceContext* deviceContext, XMMATRIX viewMatrix, XMMATRIX projectionMatrix) {
	// Render Unit Cube
	m_UnitCubeModel->Render(deviceContext);

	// Update cubemap shader buffers
	HRESULT result {};
	D3D11_MAPPED_SUBRESOURCE mappedResource {};
	MatrixBufferType* dataPtr {};
	unsigned int bufferNumber {};

	// removing translation in view matrix by truncating 4x4 matrix to 3x3
	XMFLOAT3X3 viewMatrix3x3 {};
	XMStoreFloat3x3(&viewMatrix3x3, viewMatrix);
	viewMatrix = XMLoadFloat3x3(&viewMatrix3x3);

	// Transpose the matrices to prepare them for the shader.
	viewMatrix = DirectX::XMMatrixTranspose(viewMatrix);
	projectionMatrix = DirectX::XMMatrixTranspose(projectionMatrix);

	// Lock the constant buffer so it can be written to.
	result = deviceContext->Map(m_MatrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if(FAILED(result)) {
		return false;
	}

	// Get a pointer to the data in the constant buffer.
	dataPtr = (MatrixBufferType*)mappedResource.pData;

	// Copy the matrices into the constant buffer.
	dataPtr->view = viewMatrix;
	dataPtr->projection = projectionMatrix;

	// Unlock the constant buffer.
	deviceContext->Unmap(m_MatrixBuffer, 0);

	// Set the position of the constant buffer in the vertex shader.
	bufferNumber = 0;

	// Finally set the constant buffer in the vertex shader with the updated values.
	deviceContext->VSSetConstantBuffers(bufferNumber, 1, &m_MatrixBuffer);

	// Set shader texture resource in the pixel shader.
	ID3D11ShaderResourceView* texture = m_UnitCubeModel->GetTexture(0);
	deviceContext->PSSetShaderResources(0, 1, &texture);

	/// Render cubemap shader on unit cube
	// Set the vertex input layout.
	deviceContext->IASetInputLayout(m_Layout);

	// Set the vertex and pixel shaders that will be used to render this triangle.
	deviceContext->VSSetShader(m_VertexShader, NULL, 0);
	deviceContext->PSSetShader(m_PixelShader, NULL, 0);
	// Set the sampler state in the pixel shader.
	deviceContext->PSSetSamplers(0, 1, &m_SampleState);

	// Render the triangle.
	deviceContext->DrawIndexed(m_UnitCubeModel->GetIndexCount(), 0, 0);

	return true;
}

bool CubeMapObject::InitializeShader(ID3D11Device* device, HWND hwnd, WCHAR* vsFilename, WCHAR* psFilename) {
	HRESULT result {};
	// Initialize the pointers this function will use to null.
	ID3D10Blob* errorMessage {};
	ID3D10Blob* vertexShaderBuffer {};
	ID3D10Blob* pixelShaderBuffer {};
	D3D11_INPUT_ELEMENT_DESC polygonLayout[3] {};
	unsigned int numElements {};
	D3D11_BUFFER_DESC matrixBufferDesc {};

	D3D11_SAMPLER_DESC cubeMapSamplerDesc {};

	// Compile the vertex shader code.
	result = D3DCompileFromFile(vsFilename, NULL, NULL, "CubeMapVertexShader", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
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
	result = D3DCompileFromFile(psFilename, NULL, NULL, "CubeMapPixelShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
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

	polygonLayout[2].SemanticName = "NORMAL";
	polygonLayout[2].SemanticIndex = 0;
	polygonLayout[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[2].InputSlot = 0;
	polygonLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[2].InstanceDataStepRate = 0;

	// Get a count of the elements in the layout.
	numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

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

