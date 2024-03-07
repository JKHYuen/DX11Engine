#pragma once
#include <d3d11.h>
#include <string>

class HDRTexture {
public:
    HDRTexture();
    HDRTexture(const HDRTexture&);
    ~HDRTexture();

    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const std::string& filename);
    void Shutdown();

    ID3D11ShaderResourceView* GetTextureSRV() const { return m_TextureView; };
    int GetWidth() const { return m_Width; };
    int GetHeight() const { return m_Height; };

private:
    // Temp pointer to load texture binary data (declared as member only for memory cleanup in Shutdown())
    float* m_TextureData {};

    // optionally member scoped, 
    // see https://stackoverflow.com/questions/54000030/how-when-to-release-resources-and-resource-views-in-directx
    ID3D11Texture2D* m_Texture{};

    ID3D11ShaderResourceView* m_TextureView {};
    int m_Width{};
    int m_Height{};
};