#include "HDRTexture.h"
#include "stb_image.h"

HDRTexture::HDRTexture() {}
HDRTexture::HDRTexture(const HDRTexture&) {}
HDRTexture::~HDRTexture() {}

bool HDRTexture::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const std::string& filePath) {
	HRESULT hResult{};

	int nrComponents;
	m_TextureData = stbi_loadf(filePath.c_str(), &m_Width, &m_Height, &nrComponents, 4);
	if(!m_TextureData) {
		return false;
	}

	D3D11_TEXTURE2D_DESC textureDesc {};
	textureDesc.Height = m_Height;
	textureDesc.Width = m_Width;
	textureDesc.ArraySize = 1;
	textureDesc.MipLevels = 0;
	textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	textureDesc.MiscFlags = 0;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.CPUAccessFlags = 0;

	// Create the empty texture.
	hResult = device->CreateTexture2D(&textureDesc, NULL, &m_Texture);
	if(FAILED(hResult)) {
		stbi_image_free(m_TextureData);
		return false;
	}

	deviceContext->UpdateSubresource(m_Texture, 0, NULL, m_TextureData, m_Width * 4 * sizeof(float), 0);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc {};
	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;

	hResult = device->CreateShaderResourceView(m_Texture, &srvDesc, &m_TextureView);
	if(FAILED(hResult)) {
		return false;
	}

	//deviceContext->GenerateMips(m_TextureView);

	stbi_image_free(m_TextureData);

	return true;
}

void HDRTexture::Shutdown() {
	if(m_TextureView) {
		m_TextureView->Release();
		m_TextureView = nullptr;
	}

	if(m_Texture) {
		m_Texture->Release();
		m_Texture = nullptr;
	}

	// in case of early return
	//if(m_TextureData) {
	//	stbi_image_free(m_TextureData);
	//}
}
