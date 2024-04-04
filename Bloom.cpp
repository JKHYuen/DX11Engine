#include "Bloom.h"

#include "RenderTexture.h"
#include "TextureShader.h"
#include "D3DInstance.h"

#include <d3dcompiler.h>
#include <fstream>

// default maximum number of blur iterations for bloom effect, m_CurrentMaxIteration is max of current bloom instance (depends on screen resolution)
static const int k_DefaultMaxIterations = 16;

// TODO: combine post processing (tonemapping shader) functionality from "Scene" with this class, rename this class to "PostProcess"
bool Bloom::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND hwnd, RenderTexture* screenTexture, TextureShader* screenRenderShader, TextureShader* passThroughShaderInstance) {
	bool result;

	m_Intensity = 1.0f;
	m_Threshold = 1.0f;
	m_SoftThreshold = 0.5f;
	m_UsePrefilter = true;
	m_BoxSampleDelta = 1.0f;

	m_PassThroughShaderInstance = passThroughShaderInstance;
	m_ScreenVertexShaderInstance = screenRenderShader->GetVertexShader();
	m_screenShaderLayoutInstance = screenRenderShader->GetShaderInputLayout();

	/// Generate render textures for down/up sampling
	int width = screenTexture->GetTextureWidth();
	int height = screenTexture->GetTextureHeight();
	float nearZ = screenTexture->GetNearZ();
	float farZ = screenTexture->GetFarZ();

	m_CurrentMaxIteration = k_DefaultMaxIterations;
	for(int i = 0; i < k_DefaultMaxIterations; i++) {
		m_RenderTexures.push_back(new RenderTexture());
		result = m_RenderTexures[i]->Initialize(device, deviceContext, width, height, nearZ, farZ, DXGI_FORMAT_R32G32B32A32_FLOAT);
		if(!result) return false;

		width /= 2;
		height /= 2;

		if(height < 2) {
			m_CurrentMaxIteration = i + 1;
			break;
		}
	}

	result = InitializeShader(device, hwnd, L"Bloom");
	if(!result) return false;

	return true;
}

bool Bloom::InitializeShader(ID3D11Device* device, HWND hwnd, std::wstring pixelShaderName) {
	HRESULT result {};
	ID3D10Blob* errorMessage {};
	ID3D10Blob* pixelShaderBuffer {};

	std::wstring psFileName = L"../DX11Engine/Shaders/" + pixelShaderName + L".ps";

	/// Compile the pixel shader
	result = D3DCompileFromFile(psFileName.c_str(), NULL, NULL, "BloomPixelShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
		&pixelShaderBuffer, &errorMessage);
	if(FAILED(result)) {
		if(errorMessage) {
			OutputShaderErrorMessage(errorMessage, hwnd, (WCHAR*)psFileName.c_str());
		}
		else {
			MessageBox(hwnd, psFileName.c_str(), L"Missing Shader File", MB_OK);
		}

		return false;
	}

	result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &m_PixelShader);
	if(FAILED(result)) {
		return false;
	}
	pixelShaderBuffer->Release();
	pixelShaderBuffer = nullptr;

	/// Setup matrix buffer
	D3D11_BUFFER_DESC matrixBufferDesc {};
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;

	D3D11_BUFFER_DESC shaderParamBufferDesc {};
	result = device->CreateBuffer(&matrixBufferDesc, NULL, &m_MatrixBuffer);
	if(FAILED(result)) {
		return false;
	}

	shaderParamBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	shaderParamBufferDesc.ByteWidth = sizeof(BloomParamBufferType);
	shaderParamBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	shaderParamBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	shaderParamBufferDesc.MiscFlags = 0;
	shaderParamBufferDesc.StructureByteStride = 0;

	result = device->CreateBuffer(&shaderParamBufferDesc, NULL, &m_BloomParamBuffer);
	if(FAILED(result)) {
		return false;
	}

	/// Create a texture sampler state 
	D3D11_SAMPLER_DESC samplerDesc {};
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

bool Bloom::RenderEffect(D3DInstance* d3dInstance , int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ID3D11ShaderResourceView* textureSRV) {
	// downsample
	m_BoxSampleDelta = 1.0f;
	m_UsePrefilter = true;
	m_RenderTexures[3]->SetRenderTargetAndViewPort();
	m_RenderTexures[3]->ClearRenderTarget(1.0f, 0.0f, 0.0f, 1.0f);
	d3dInstance->DisableAlphaBlending();
	Render(d3dInstance->GetDeviceContext(), indexCount, worldMatrix, viewMatrix, projectionMatrix, textureSRV);

	// upsample
	m_UsePrefilter = false;
	m_BoxSampleDelta = 0.5f;
	m_RenderTexures[0]->SetRenderTargetAndViewPort();
	m_RenderTexures[0]->ClearRenderTarget(0.0f, 0.0f, 0.0f, 1.0f);
	d3dInstance->EnableAdditiveBlending();
	Render(d3dInstance->GetDeviceContext(), indexCount, worldMatrix, viewMatrix, projectionMatrix, m_RenderTexures[3]->GetTextureSRV());

	// TEMP
	d3dInstance->EnableAlphaBlending();
	d3dInstance->SetToBackBufferRenderTargetAndViewPort();
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

	MatrixBufferType* matrixDataPtr = (MatrixBufferType*)mappedResource.pData;

	matrixDataPtr->world = worldMatrix;
	matrixDataPtr->view = viewMatrix;
	matrixDataPtr->projection = projectionMatrix;

	deviceContext->Unmap(m_MatrixBuffer, 0);

	deviceContext->VSSetConstantBuffers(0, 1, &m_MatrixBuffer);

	/// Write to shader parameters buffer
	/// TODO: only do this when params change
	result = deviceContext->Map(m_BloomParamBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if(FAILED(result)) return false;

	BloomParamBufferType* bloomDataPtr = (BloomParamBufferType*)mappedResource.pData;

	float knee = m_Threshold * m_SoftThreshold;
	XMFLOAT4 filter {m_Threshold, m_Threshold - knee, 2.0f * knee, 0.25f / (knee + 0.00001f)};
	bloomDataPtr->filter         = filter;
	bloomDataPtr->boxSampleDelta = m_BoxSampleDelta;
	bloomDataPtr->intensity      = m_Intensity;
	bloomDataPtr->usePrefilter   = m_UsePrefilter ? 1.0f : 0.0f;

	deviceContext->Unmap(m_BloomParamBuffer, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &m_BloomParamBuffer);

	/// Bind Textures
	deviceContext->PSSetShaderResources(0, 1, &textureSRV);

	/// Render the prepared buffers with the shader
	deviceContext->IASetInputLayout(m_screenShaderLayoutInstance);

	deviceContext->VSSetShader(m_ScreenVertexShaderInstance, NULL, 0);

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

// Note: do not cleanup vertex shader or input layout instances, they are passed in by screen shader
void Bloom::Shutdown() {
	if(m_SampleState) {
		m_SampleState->Release();
		m_SampleState = nullptr;
	}

	if(m_MatrixBuffer) {
		m_MatrixBuffer->Release();
		m_MatrixBuffer = nullptr;
	}

	if(m_BloomParamBuffer) {
		m_BloomParamBuffer->Release();
		m_BloomParamBuffer = nullptr;
	}

	if(m_PixelShader) {
		m_PixelShader->Release();
		m_PixelShader = nullptr;
	}

	for(RenderTexture* rt : m_RenderTexures) {
		rt->Shutdown();
		delete rt;
		rt = nullptr;
	}
}
