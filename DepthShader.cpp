#include "DepthShader.h"
#include "Texture.h"
#include "ShaderUtil.h"

DepthShader::DepthShader() {};
DepthShader::DepthShader(const DepthShader&) {};
DepthShader::~DepthShader() {};

bool DepthShader::Initialize(ID3D11Device* device, HWND hwnd) {
	const std::wstring vsFileName = L"./shaders/Depth.vs";
	const std::wstring psFileName = L"./shaders/Depth.ps";
	const std::wstring hsFileName = L"./shaders/Depth.hs";
	const std::wstring dsFileName = L"./shaders/Depth.ds";

	HRESULT result {};

	/// Compile and create shaders
	ID3D10Blob* errorMessage {};
	ID3D10Blob* vertexShaderBuffer {};
	ID3D10Blob* pixelShaderBuffer {};
	ID3D10Blob* hullShaderBuffer {};
	ID3D10Blob* domainShaderBuffer {};

	result = D3DCompileFromFile(vsFileName.c_str(), NULL, NULL, "DepthVertexShader", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
		&vertexShaderBuffer, &errorMessage);
	if(FAILED(result)) {
		// If the shader failed to compile it should have writen something to the error message.
		if(errorMessage) {
			OutputShaderErrorMessage(errorMessage, hwnd, vsFileName.c_str());
		}
		// If there was  nothing in the error message then it simply could not find the shader file itself.
		else {
			MessageBox(hwnd, vsFileName.c_str(), L"Missing Shader File", MB_OK);
		}

		return false;
	}
	result = D3DCompileFromFile(psFileName.c_str(), NULL, NULL, "DepthPixelShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
		&pixelShaderBuffer, &errorMessage);
	if(FAILED(result)) {
		// If the shader failed to compile it should have writen something to the error message.
		if(errorMessage) {
			OutputShaderErrorMessage(errorMessage, hwnd, psFileName.c_str());
		}
		// If there was nothing in the error message then it simply could not find the file itself.
		else {
			MessageBox(hwnd, psFileName.c_str(), L"Missing Shader File", MB_OK);
		}

		return false;
	}
	result = D3DCompileFromFile(hsFileName.c_str(), NULL, NULL, "DepthHullShader", "hs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &hullShaderBuffer, &errorMessage);
	if(FAILED(result)) {
		if(errorMessage) {
			OutputShaderErrorMessage(errorMessage, hwnd, hsFileName.c_str());
		}
		else {
			MessageBox(hwnd, hsFileName.c_str(), L"Missing Shader File", MB_OK);
		}
		return false;
	}
	result = D3DCompileFromFile(dsFileName.c_str(), NULL, NULL, "DepthDomainShader", "ds_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &domainShaderBuffer, &errorMessage);
	if(FAILED(result)) {
		if(errorMessage) {
			OutputShaderErrorMessage(errorMessage, hwnd, dsFileName.c_str());
		}
		else {
			MessageBox(hwnd, dsFileName.c_str(), L"Missing Shader File", MB_OK);
		}
		return false;
	}

	result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &m_VertexShader);
	if(FAILED(result)) return false;
	result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &m_PixelShader);
	if(FAILED(result)) return false;
	result = device->CreateHullShader(hullShaderBuffer->GetBufferPointer(), hullShaderBuffer->GetBufferSize(), NULL, &m_HullShader);
	if(FAILED(result)) return false;
	result = device->CreateDomainShader(domainShaderBuffer->GetBufferPointer(), domainShaderBuffer->GetBufferSize(), NULL, &m_DomainShader);
	if(FAILED(result)) return false;

	// Create the vertex input layout description.
	D3D11_INPUT_ELEMENT_DESC polygonLayout[3] {};
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

	// Create the vertex input layout.
	unsigned int numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);
	result = device->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &m_Layout);
	if(FAILED(result)) {
		return false;
	}

	vertexShaderBuffer->Release();
	vertexShaderBuffer = nullptr;

	pixelShaderBuffer->Release();
	pixelShaderBuffer = nullptr;

	hullShaderBuffer->Release();
	hullShaderBuffer = nullptr;

	domainShaderBuffer->Release();
	domainShaderBuffer = nullptr;

	/// Create the texture sampler state
	D3D11_SAMPLER_DESC samplerDesc {};
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	result = device->CreateSamplerState(&samplerDesc, &m_SampleStateWrap);
	if(FAILED(result)) {
		return false;
	}

	/// Setup matrix buffer
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

	/// Setup depth material buffer
	D3D11_BUFFER_DESC depthMaterialBufferDesc {};
	depthMaterialBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	depthMaterialBufferDesc.ByteWidth = sizeof(DepthMaterialBufferType);
	depthMaterialBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	depthMaterialBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	depthMaterialBufferDesc.MiscFlags = 0;
	depthMaterialBufferDesc.StructureByteStride = 0;

	result = device->CreateBuffer(&depthMaterialBufferDesc, NULL, &m_DepthMaterialBuffer);
	if(FAILED(result)) {
		return false;
	}

	/// Setup tessellation buffer
	D3D11_BUFFER_DESC tessellationBufferDesc {};
	tessellationBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	tessellationBufferDesc.ByteWidth = sizeof(TessellationBufferType);
	tessellationBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	tessellationBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	tessellationBufferDesc.MiscFlags = 0;
	tessellationBufferDesc.StructureByteStride = 0;

	result = device->CreateBuffer(&tessellationBufferDesc, NULL, &m_TessellationBuffer);
	if(FAILED(result)) {
		return false;
	}

	return true;
}

bool DepthShader::Render(ID3D11DeviceContext* deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix,
	XMMATRIX projectionMatrix, Texture* heightMap, const GameObject::GameObjectData& gameObjectData) {
	HRESULT result {};
	D3D11_MAPPED_SUBRESOURCE mappedResource {};

	// Transpose the matrices to prepare them for the shader
	worldMatrix = XMMatrixTranspose(worldMatrix);
	viewMatrix = XMMatrixTranspose(viewMatrix);
	projectionMatrix = XMMatrixTranspose(projectionMatrix);

	/// Update DS buffers
	result = deviceContext->Map(m_MatrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if(FAILED(result)) {
		return false;
	}

	MatrixBufferType* matrixDataPtr {};
	matrixDataPtr = (MatrixBufferType*)mappedResource.pData;

	matrixDataPtr->world = worldMatrix;
	matrixDataPtr->view = viewMatrix;
	matrixDataPtr->projection = projectionMatrix;

	deviceContext->Unmap(m_MatrixBuffer, 0);
	deviceContext->DSSetConstantBuffers(0, 1, &m_MatrixBuffer);

	// Depth material buffer
	result = deviceContext->Map(m_DepthMaterialBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if(FAILED(result)) {
		return false;
	}

	DepthMaterialBufferType* depthMaterialDataPtr {};
	depthMaterialDataPtr = (DepthMaterialBufferType*)mappedResource.pData;

	depthMaterialDataPtr->heightMapScale = gameObjectData.vertexDisplacementMapScale;
	depthMaterialDataPtr->uvScale = gameObjectData.uvScale;
	depthMaterialDataPtr->padding = {};

	deviceContext->Unmap(m_DepthMaterialBuffer, 0);
	deviceContext->DSSetConstantBuffers(1, 1, &m_DepthMaterialBuffer);

	/// Bind Domain Shader Textures
	ID3D11ShaderResourceView* pTempSRV = heightMap->GetTextureSRV(); // height map
	deviceContext->DSSetShaderResources(0, 1, &pTempSRV);

	/// Update HS buffers
	result = deviceContext->Map(m_TessellationBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if(FAILED(result)) {
		return false;
	}

	TessellationBufferType* tessellationDataPtr {};
	tessellationDataPtr = (TessellationBufferType*)mappedResource.pData;

	tessellationDataPtr->tessellationFactor = gameObjectData.uniformTessellationFactor;
	tessellationDataPtr->padding = {};

	deviceContext->Unmap(m_TessellationBuffer, 0);
	deviceContext->HSSetConstantBuffers(0, 1, &m_TessellationBuffer);

	/// Render
	deviceContext->IASetInputLayout(m_Layout);

	deviceContext->VSSetShader(m_VertexShader, NULL, 0);
	deviceContext->PSSetShader(m_PixelShader, NULL, 0);
	deviceContext->HSSetShader(m_HullShader, NULL, 0);
	deviceContext->DSSetShader(m_DomainShader, NULL, 0);

	deviceContext->DSSetSamplers(0, 1, &m_SampleStateWrap);

	deviceContext->DrawIndexed(indexCount, 0, 0);

	return true;
}

void DepthShader::Shutdown() {
	if(m_MatrixBuffer) {
		m_MatrixBuffer->Release();
		m_MatrixBuffer = nullptr;
	}

	if(m_DepthMaterialBuffer) {
		m_DepthMaterialBuffer->Release();
		m_DepthMaterialBuffer = nullptr;
	}

	if(m_TessellationBuffer) {
		m_TessellationBuffer->Release();
		m_TessellationBuffer = nullptr;
	}

	if(m_SampleStateWrap) {
		m_SampleStateWrap->Release();
		m_SampleStateWrap = nullptr;
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

	if(m_HullShader) {
		m_HullShader->Release();
		m_HullShader = nullptr;
	}

	if(m_DomainShader) {
		m_DomainShader->Release();
		m_DomainShader = nullptr;
	}
}
