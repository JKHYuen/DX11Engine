#pragma once

#include <d3d11.h>
#include <stdio.h>

#include <array>
#include <string>

class TextureClass {
public:
    TextureClass();
    TextureClass(const TextureClass&);
    ~TextureClass();

    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const std::string& filename, bool isCubeMap = false);
    void Shutdown();

    ID3D11ShaderResourceView* GetTextureSRV();

    int GetWidth();
    int GetHeight();

private:
    // Used in LoadTarga32Bit()
    struct TargaHeader {
        unsigned char data1[12];
        unsigned short width;
        unsigned short height;
        unsigned char bpp;
        unsigned char data2;
    };

    // Ordered texture file names of 6 cubemap faces
    static const inline std::array<std::string, 6> kCubeMapFaceName = {"right", "left", "top", "bottom", "back", "front"};

    // Bandaid solution to check if we are using STB lib
    bool m_IsSTBLoad {};

private:
    static bool LoadTarga32Bit(const char* filename, unsigned char** pData, int& width, int& height);

    // loads file from filePath and creates ID3D11Texture2D; pTexture will point to created texture
    bool LoadTexture(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const std::string& filePath, DXGI_FORMAT format, ID3D11Texture2D** pTexture);

    // load single texture from file, store results into m_Texture and m_TextureView
    bool InitializeTexture(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const std::string& filePath);

    // load cubemap (folderPath contains folder of 6 textures with filenames in kCubeMapFaceName)
    bool InitializeCubeMap(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const std::string& folderPath);

    // Temp pointer to load texture binary data (declared as member only for memory cleanup in Shutdown())
    unsigned char* m_UCharTexData {};

    // optionally member scoped, 
    // see https://stackoverflow.com/questions/54000030/how-when-to-release-resources-and-resource-views-in-directx
    ID3D11Texture2D* m_Texture {};
    std::array<ID3D11Texture2D*, 6> m_CubeMapSourceTextureArray {};

    ID3D11ShaderResourceView* m_TextureView {};
    int m_Width {};
    int m_Height {};
};