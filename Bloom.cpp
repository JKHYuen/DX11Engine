#include "Bloom.h"

#include "RenderTexture.h"

#include <d3dcompiler.h>
#include <fstream>

// TODO: include tonemapping, rename class to "PostProcess"
bool Bloom::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND hwnd, RenderTexture* screenTexture, ID3D11VertexShader* screenRenderVertexShader) {
	m_VertexShader = screenRenderVertexShader;

	int width = screenTexture->GetTextureWidth();
	int height = screenTexture->GetTextureHeight();
	float nearZ = screenTexture->GetNearZ();
	float farZ = screenTexture->GetFarZ();
	for(size_t i = 0; i < m_RenderTexArray.size(); i++) {
		m_RenderTexArray[i] = new RenderTexture();
		if(m_RenderTexArray[i]->Initialize(device, deviceContext, width, height, nearZ, farZ, DXGI_FORMAT_R32G32B32A32_FLOAT)) {
			return false;
		}

		width /= 2;
		height /= 2;
	}

	bool result = InitializeShader(device, hwnd, L"BoxSample");
	if(!result) return false;

	return true;
}

bool Bloom::InitializeShader(ID3D11Device* device, HWND hwnd, std::wstring pixelShaderName) {
	HRESULT result {};
	ID3D10Blob* errorMessage {};
	ID3D10Blob* vertexShaderBuffer {};
	ID3D10Blob* pixelShaderBuffer {};

	std::wstring psFileName = L"../DX11Engine/Shaders/" + pixelShaderName + L".ps";

	// Compile the pixel shader code.
	result = D3DCompileFromFile(psFileName.c_str(), NULL, NULL, "PixelShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
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

	// Create the pixel shader from the buffer.
	result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &m_PixelShader);
	if(FAILED(result)) {
		return false;
	}

	D3D11_INPUT_ELEMENT_DESC polygonLayout[2] {};
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

	D3D11_BUFFER_DESC matrixBufferDesc {};
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

	D3D11_SAMPLER_DESC samplerDesc {};
	// Create a texture sampler state description.
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	// Create the texture sampler state.
	result = device->CreateSamplerState(&samplerDesc, &m_SampleState);
	if(FAILED(result)) {
		return false;
	}

	return true;
}

bool Bloom::Render(ID3D11DeviceContext* deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ID3D11ShaderResourceView* textureSRV) {
	// Transpose the matrices to prepare them for the shader.
	worldMatrix = XMMatrixTranspose(worldMatrix);
	viewMatrix = XMMatrixTranspose(viewMatrix);
	projectionMatrix = XMMatrixTranspose(projectionMatrix);

	/// Write to matrix buffer
	D3D11_MAPPED_SUBRESOURCE mappedResource {};
	HRESULT result = deviceContext->Map(m_MatrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if(FAILED(result)) {
		return false;
	}

	MatrixBufferType* dataPtr = (MatrixBufferType*)mappedResource.pData;

	dataPtr->world = worldMatrix;
	dataPtr->view = viewMatrix;
	dataPtr->projection = projectionMatrix;

	deviceContext->Unmap(m_MatrixBuffer, 0);

	deviceContext->VSSetConstantBuffers(0, 1, &m_MatrixBuffer);
	deviceContext->PSSetShaderResources(0, 1, &textureSRV);

	/// Render the prepared buffers with the shader
	deviceContext->IASetInputLayout(m_Layout);

	deviceContext->VSSetShader(m_VertexShader, NULL, 0);
	deviceContext->PSSetShader(m_PixelShader, NULL, 0);
	deviceContext->PSSetSamplers(0, 1, &m_SampleState);

	deviceContext->DrawIndexed(indexCount, 0, 0);

	return true;
}

void Bloom::OutputShaderErrorMessage(ID3D10Blob* errorMessage, HWND hwnd, WCHAR* shaderFilename) {
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
}


void Bloom::Shutdown() {
	if(m_SampleState) {
		m_SampleState->Release();
		m_SampleState = nullptr;
	}

	if(m_MatrixBuffer) {
		m_MatrixBuffer->Release();
		m_MatrixBuffer = nullptr;
	}

	if(m_Layout) {
		m_Layout->Release();
		m_Layout = nullptr;
	}

	if(m_PixelShader) {
		m_PixelShader->Release();
		m_PixelShader = nullptr;
	}

	if(m_VertexShader) {
		m_VertexShader->Release();
		m_VertexShader = nullptr;
	}
}
