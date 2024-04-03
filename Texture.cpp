#include "Texture.h"
#include "stb_image.h"

#include <stdio.h>

bool Texture::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const std::string& filePath, DXGI_FORMAT format, int mipLevels) {
	bool b_GenerateMips = mipLevels == 0 || mipLevels > 1;

	/// Load texture from disk
	/// NOTE: use rastertek loader if tga file, else, stb_image; because rastertek function seems to be significantly faster
	std::string fileTypeName{ filePath, filePath.length() - 3, 3 };
	if(fileTypeName == "tga") {
		m_IsSTBLoad = false;
		// loads data to m_UCharTexData, m_Width and m_Height
		bool result = LoadTarga32Bit(filePath.c_str(), &m_TempUCharTexData, m_Width, m_Height);
		if(!result) {
			return false;
		}
	}
	else if(fileTypeName == "hdr") {
		m_IsSTBLoad = true;
		int nrComponents;
		m_TempFloatTexData = stbi_loadf(filePath.c_str(), &m_Width, &m_Height, &nrComponents, 4);
		if(!m_TempFloatTexData) {
			return false;
		}
	}
	else {
		m_IsSTBLoad = true;
		int nrComponents;
		m_TempUCharTexData = stbi_load(filePath.c_str(), &m_Width, &m_Height, &nrComponents, 4);
		if(!m_TempUCharTexData) {
			return false;
		}
	}

	// Setup the description of the texture.
	D3D11_TEXTURE2D_DESC textureDesc{};
	textureDesc.Height = m_Height;
	textureDesc.Width = m_Width;
	textureDesc.ArraySize = 1;
	textureDesc.MipLevels = mipLevels;
	textureDesc.Format = format;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	textureDesc.MiscFlags = 0;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.CPUAccessFlags = 0;

	if(b_GenerateMips) {
		textureDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
	}

	// Create the empty texture.
	HRESULT hResult = device->CreateTexture2D(&textureDesc, NULL, &m_Texture);
	if(FAILED(hResult)) {
		return false;
	}

	// uchar texture load and clean up
	if(m_TempUCharTexData) {
		deviceContext->UpdateSubresource(m_Texture, 0, NULL, m_TempUCharTexData, m_Width * 4 * sizeof(unsigned char), 0);
		if(m_IsSTBLoad) {
			stbi_image_free(m_TempUCharTexData);
		}
		else {
			delete[] m_TempUCharTexData;
		}
		m_TempUCharTexData = nullptr;
	}
	// float texture load
	else if(m_TempFloatTexData){
		deviceContext->UpdateSubresource(m_Texture, 0, NULL, m_TempFloatTexData, m_Width * 4 * sizeof(float), 0);
		stbi_image_free(m_TempFloatTexData);
		m_TempFloatTexData = nullptr;
	}

	/// Setup the shader resource view description.
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc {};
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;

	// Create the shader resource view for the texture.
	hResult = device->CreateShaderResourceView(m_Texture, &srvDesc, &m_TextureView);
	if(FAILED(hResult)) {
		return false;
	}

	if(b_GenerateMips) {
		deviceContext->GenerateMips(m_TextureView);
	}

	return true;
}

bool Texture::AsyncInitialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const std::string& filePath, DXGI_FORMAT format, int mipLevels, std::mutex deviceContextMutex) {
	bool b_GenerateMips = mipLevels == 0 || mipLevels > 1;

	/// Load texture from disk
	/// NOTE: use rastertek loader if tga file, else, stb_image; because rastertek function seems to be significantly faster
	std::string fileTypeName {filePath, filePath.length() - 3, 3};
	if(fileTypeName == "tga") {
		m_IsSTBLoad = false;
		// loads data to m_UCharTexData, m_Width and m_Height
		bool result = LoadTarga32Bit(filePath.c_str(), &m_TempUCharTexData, m_Width, m_Height);
		if(!result) {
			return false;
		}
	}
	else if(fileTypeName == "hdr") {
		m_IsSTBLoad = true;
		int nrComponents;
		m_TempFloatTexData = stbi_loadf(filePath.c_str(), &m_Width, &m_Height, &nrComponents, 4);
		if(!m_TempFloatTexData) {
			return false;
		}
	}
	else {
		m_IsSTBLoad = true;
		int nrComponents;
		m_TempUCharTexData = stbi_load(filePath.c_str(), &m_Width, &m_Height, &nrComponents, 4);
		if(!m_TempUCharTexData) {
			return false;
		}
	}

	// Setup the description of the texture.
	D3D11_TEXTURE2D_DESC textureDesc {};
	textureDesc.Height = m_Height;
	textureDesc.Width = m_Width;
	textureDesc.ArraySize = 1;
	textureDesc.MipLevels = mipLevels;
	textureDesc.Format = format;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	textureDesc.MiscFlags = 0;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.CPUAccessFlags = 0;

	if(b_GenerateMips) {
		textureDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
	}

	// Create the empty texture.
	HRESULT hResult = device->CreateTexture2D(&textureDesc, NULL, &m_Texture);
	if(FAILED(hResult)) {
		return false;
	}

	{
		std::lock_guard<std::mutex> lock {deviceContextMutex};
		// uchar texture load and clean up
		if(m_TempUCharTexData) {
			deviceContext->UpdateSubresource(m_Texture, 0, NULL, m_TempUCharTexData, m_Width * 4 * sizeof(unsigned char), 0);
			if(m_IsSTBLoad) {
				stbi_image_free(m_TempUCharTexData);
			}
			else {
				delete[] m_TempUCharTexData;
			}
			m_TempUCharTexData = nullptr;
		}
		// float texture load
		else if(m_TempFloatTexData) {
			deviceContext->UpdateSubresource(m_Texture, 0, NULL, m_TempFloatTexData, m_Width * 4 * sizeof(float), 0);
			stbi_image_free(m_TempFloatTexData);
			m_TempFloatTexData = nullptr;
		}
	}


	/// Setup the shader resource view description.
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc {};
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;

	// Create the shader resource view for the texture.
	hResult = device->CreateShaderResourceView(m_Texture, &srvDesc, &m_TextureView);
	if(FAILED(hResult)) {
		return false;
	}

	if(b_GenerateMips) {
		std::lock_guard<std::mutex> lock {deviceContextMutex};
		deviceContext->GenerateMips(m_TextureView);
	}

	return true;
}

// NOTE: currently unused, can be used to load 6 textures on disk into a cubemap srv
bool Texture::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const std::array<ID3D11Texture2D*, 6>& sourceHDRTexArray) {
	D3D11_TEXTURE2D_DESC texElementDesc;
	sourceHDRTexArray[0]->GetDesc(&texElementDesc);

	D3D11_TEXTURE2D_DESC textureArrayDesc {};
	textureArrayDesc.Height = texElementDesc.Height;
	textureArrayDesc.Width = texElementDesc.Width;
	textureArrayDesc.ArraySize = 6;
	textureArrayDesc.Format = texElementDesc.Format;
	textureArrayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	textureArrayDesc.MipLevels = texElementDesc.MipLevels;
	textureArrayDesc.SampleDesc.Count = 1;
	textureArrayDesc.SampleDesc.Quality = 0;
	textureArrayDesc.Usage = D3D11_USAGE_DEFAULT;
	textureArrayDesc.CPUAccessFlags = 0;
	textureArrayDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;

	// Initialize empty texture array
	ID3D11Texture2D* texArray = nullptr;
	HRESULT hResult = device->CreateTexture2D(&textureArrayDesc, 0, &texArray);
	if(FAILED(hResult)) {
		return false;
	}

	// Source: https://stackoverflow.com/a/34325668
	// Copy individual texture elements into texture array.
	D3D11_BOX sourceRegion{};
	for(UINT x = 0; x < 6; x++) {
		for(UINT mipLevel = 0; mipLevel < textureArrayDesc.MipLevels; mipLevel++) {
			sourceRegion.left = 0;
			sourceRegion.right = (textureArrayDesc.Width >> mipLevel);
			sourceRegion.top = 0;
			sourceRegion.bottom = (textureArrayDesc.Height >> mipLevel);
			sourceRegion.front = 0;
			sourceRegion.back = 1;

			//test for overflow
			if(sourceRegion.bottom == 0 || sourceRegion.right == 0)
				break;

			deviceContext->CopySubresourceRegion(texArray, D3D11CalcSubresource(mipLevel, x, textureArrayDesc.MipLevels), 0, 0, 0, sourceHDRTexArray[x], mipLevel, &sourceRegion);
		}
	}

	// Setup the shader resource view description.
	D3D11_SHADER_RESOURCE_VIEW_DESC cubeMapSrvDesc{};
	cubeMapSrvDesc.Format = textureArrayDesc.Format;
	cubeMapSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	cubeMapSrvDesc.TextureCube.MostDetailedMip = 0;
	cubeMapSrvDesc.TextureCube.MipLevels = textureArrayDesc.MipLevels;

	// Create the shader resource view for the texture.
	hResult = device->CreateShaderResourceView(texArray, &cubeMapSrvDesc, &m_TextureView);
	if(FAILED(hResult)) {
		return false;
	}

	deviceContext->GenerateMips(m_TextureView);

	return true;
}


bool Texture::LoadTarga32Bit(const char* filename, unsigned char** pData, int& width, int& height) {
	int error {}, bpp {}, imageSize {}, index {}, i {}, j {}, k {};
	FILE* filePtr {};
	unsigned int count {};
	TargaHeader targaFileHeader {};
	unsigned char* targaImage {};

	// Open the targa file for reading in binary.
	error = fopen_s(&filePtr, filename, "rb");
	if(error != 0) {
		return false;
	}

	// Read in the file header.
	count = (unsigned int)fread(&targaFileHeader, sizeof(TargaHeader), 1, filePtr);
	if(count != 1) {
		return false;
	}

	// Get the important information from the header.
	height = (int)targaFileHeader.height;
	width = (int)targaFileHeader.width;
	bpp = (int)targaFileHeader.bpp;

	// Check that it is 32 bit and not 24 bit.
	if(bpp != 32) {
		return false;
	}

	// Calculate the size of the 32 bit image data.
	imageSize = width * height * 4;

	// Allocate memory for the targa image data.
	targaImage = new unsigned char[imageSize];

	// Read in the targa image data.
	count = (unsigned int)fread(targaImage, 1, imageSize, filePtr);
	if(count != imageSize) {
		return false;
	}

	// Close the file.
	error = fclose(filePtr);
	if(error != 0) {
		return false;
	}

	// Allocate memory for the targa destination data.
	*pData = new unsigned char[imageSize];

	// Initialize the index into the targa destination data array.
	index = 0;

	// Initialize the index into the targa image data.
	k = (width * height * 4) - (width * 4);

	// Now copy the targa image data into the targa destination array in the correct order since the targa format is stored upside down and also is not in RGBA order.
	for(j = 0; j < height; j++) {
		for(i = 0; i < width; i++) {
			(*pData)[index + 0] = targaImage[k + 2];  // Red.
			(*pData)[index + 1] = targaImage[k + 1];  // Green.
			(*pData)[index + 2] = targaImage[k + 0];  // Blue
			(*pData)[index + 3] = targaImage[k + 3];  // Alpha

			// Increment the indexes into the targa data.
			k += 4;
			index += 4;
		}

		// Set the targa image data index back to the preceding row at the beginning of the column since its reading it in upside down.
		k -= (width * 8);
	}

	// Release the targa image data now that it was copied into the destination array.
	delete[] targaImage;
	targaImage = nullptr;

	return true;
}

int Texture::GetWidth() {
	return m_Width;
}

int Texture::GetHeight() {
	return m_Height;
}

void Texture::Shutdown() {
	if(m_TextureView) {
		m_TextureView->Release();
		m_TextureView = nullptr;
	}

	if(m_Texture) {
		m_Texture->Release();
		m_Texture = nullptr;
	}

	// in case of early return
	if(m_TempUCharTexData) {
		if(m_IsSTBLoad) {
			stbi_image_free(m_TempUCharTexData);
		}
		else {
			delete[] m_TempUCharTexData;
		}
		m_TempUCharTexData = nullptr;
	}

	if(m_TempFloatTexData) {
		stbi_image_free(m_TempFloatTexData);
		m_TempFloatTexData = nullptr;
	}
}

