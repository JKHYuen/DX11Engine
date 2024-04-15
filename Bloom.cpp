#include "Bloom.h"
#include "ShaderUtil.h"

#include "RenderTexture.h"
#include "TextureShader.h"
#include "D3DInstance.h"

namespace {
	// default maximum number of down/upsample iterations for bloom effect, m_IterationCount is number of iterations to use in current bloom instance (depends on screen resolution)
	const int k_DefaultMaxIterations = 16;
}

bool Bloom::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND hwnd, RenderTexture* screenTexture, TextureShader* screenRenderShader) {
	m_ScreenVertexShaderInstance = screenRenderShader->GetVertexShader();
	m_screenShaderLayoutInstance = screenRenderShader->GetShaderInputLayout();

	/// Generate render textures for down/up sampling
	int width = screenTexture->GetTextureWidth();
	int height = screenTexture->GetTextureHeight();
	float nearZ = screenTexture->GetNearZ();
	float farZ = screenTexture->GetFarZ();

	m_BloomOutputTexture = new RenderTexture();
	bool result = m_BloomOutputTexture->Initialize(device, deviceContext, width, height, nearZ, farZ, DXGI_FORMAT_R32G32B32A32_FLOAT);
	if(!result) return false;

	m_IterationCount = k_DefaultMaxIterations;
	for(int i = 0; i < k_DefaultMaxIterations; i++) {
		m_RenderTexures.push_back(new RenderTexture());
		result = m_RenderTexures[i]->Initialize(device, deviceContext, width, height, nearZ, farZ, DXGI_FORMAT_R32G32B32A32_FLOAT);
		if(!result) return false;

		width /= 2;
		height /= 2;

		// Note: should check min of height and width 
		if(height < 2) {
			m_IterationCount = i + 1;
			break;
		}
	}

	HRESULT hresult {};
	ID3D10Blob* errorMessage {};
	ID3D10Blob* pixelShaderBuffer {};
	const WCHAR* psFileName = L"../DX11Engine/Shaders/Bloom.ps";
	/// Compile the pixel shader
	hresult = D3DCompileFromFile(psFileName, NULL, NULL, "BloomPixelShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
		&pixelShaderBuffer, &errorMessage);
	if(FAILED(hresult)) {
		if(errorMessage) {
			OutputShaderErrorMessage(errorMessage, hwnd, psFileName);
		}
		else {
			MessageBox(hwnd, psFileName, L"Missing Shader File", MB_OK);
		}

		return false;
	}

	hresult = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &m_PixelShader);
	if(FAILED(hresult)) {
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
	hresult = device->CreateBuffer(&matrixBufferDesc, NULL, &m_MatrixBuffer);
	if(FAILED(hresult)) {
		return false;
	}

	/// Setup param buffer
	shaderParamBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	shaderParamBufferDesc.ByteWidth = sizeof(BloomParamBufferType);
	shaderParamBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	shaderParamBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	shaderParamBufferDesc.MiscFlags = 0;
	shaderParamBufferDesc.StructureByteStride = 0;

	hresult = device->CreateBuffer(&shaderParamBufferDesc, NULL, &m_BloomParamBuffer);
	if(FAILED(hresult)) {
		return false;
	}

	/// Create a texture sampler state 
	D3D11_SAMPLER_DESC samplerDesc {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = 0;

	hresult = device->CreateSamplerState(&samplerDesc, &m_SampleState);
	if(FAILED(hresult)) {
		return false;
	}

	return true;
}

bool Bloom::RenderEffect(D3DInstance* d3dInstance , int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ID3D11ShaderResourceView* screenTextureSource) {
	bool result {};
	
	static ID3D11ShaderResourceView* nullSRV[1] = {nullptr};

	// First downsample + prefilter
	d3dInstance->DisableAlphaBlending();
	mb_UsePrefilter = true;
	m_BoxSampleDelta = 1.0f;

	m_RenderTexures[0]->SetRenderTargetAndViewPort();
	m_RenderTexures[0]->ClearRenderTarget(1.0f, 0.0f, 0.0f, 1.0f);
	result = Render(d3dInstance->GetDeviceContext(), indexCount, worldMatrix, viewMatrix, projectionMatrix, screenTextureSource, false);

	if(!result) return false;

	// Progressive Downsampling
	mb_UsePrefilter = false;
	int i = 1;
	for(; i < m_IterationCount; i++) {
		m_RenderTexures[i]->SetRenderTargetAndViewPort();
		m_RenderTexures[i]->ClearRenderTarget(1.0f, 0.0f, 0.0f, 1.0f);
		result = Render(d3dInstance->GetDeviceContext(), indexCount, worldMatrix, viewMatrix, projectionMatrix, m_RenderTexures[i - 1]->GetTextureSRV(), false);
		d3dInstance->GetDeviceContext()->PSSetShaderResources(0, 1, nullSRV);

		if(!result) return false;
	}

	// Progressive Upsampling
	d3dInstance->EnableAdditiveBlending();
	m_BoxSampleDelta = 0.5f;
	for(i -= 2; i >= 0; i--) {
		m_RenderTexures[i]->SetRenderTargetAndViewPort();
		result = Render(d3dInstance->GetDeviceContext(), indexCount, worldMatrix, viewMatrix, projectionMatrix, m_RenderTexures[i + 1]->GetTextureSRV(), false);
		d3dInstance->GetDeviceContext()->PSSetShaderResources(0, 1, nullSRV);

		if(!result) return false;
	}

	d3dInstance->EnableAlphaBlending();
	m_BloomOutputTexture->SetRenderTargetAndViewPort();
	m_BloomOutputTexture->ClearRenderTarget(1.0f, 0.0f, 0.0f, 1.0f);
	// Bind original screen render image
	d3dInstance->GetDeviceContext()->PSSetShaderResources(1, 1, &screenTextureSource);
	result = Render(d3dInstance->GetDeviceContext(), indexCount, worldMatrix, viewMatrix, projectionMatrix, m_RenderTexures[0]->GetTextureSRV(), true);
	if(!result) return false;

	return true;
}

bool Bloom::Render(ID3D11DeviceContext* deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ID3D11ShaderResourceView* textureSRV, bool b_IsFinalPass) const {
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
	bloomDataPtr->b_UsePrefilter = mb_UsePrefilter ? 1.0f : 0.0f;
	bloomDataPtr->b_UseFinalPass = b_IsFinalPass   ? 1.0f : 0.0f;

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

// Note: do not cleanup vertex shader or input layout instances, they are passed in by screen shader and are managed externally
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

	if(m_BloomOutputTexture) {
		m_BloomOutputTexture->Shutdown();
		delete m_BloomOutputTexture;
		m_BloomOutputTexture = nullptr;
	}

	for(RenderTexture* rt : m_RenderTexures) {
		rt->Shutdown();
		delete rt;
		rt = nullptr;
	}
}
