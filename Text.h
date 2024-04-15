#pragma once

#include "Font.h"

class Text {
private:
    struct VertexType {
        XMFLOAT3 position;
        XMFLOAT2 texture;
    };

public:
    Text() {}
    Text(const Text&) {}
    ~Text() {}

    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, int screenWidth, int screenHeight, int maxLength, Font* Font, char* text, int positionX, int positionY, float red, float green, float blue);
    void Shutdown();
    void Render(ID3D11DeviceContext* deviceContext);

    int GetIndexCount();

    bool UpdateText(ID3D11DeviceContext* deviceContext, Font* Font, char* text, int positionX, int positionY, float red, float green, float blue);
    XMFLOAT4 GetPixelColor();

    void SetScreenDimensions(int newWidth, int newHeight) { 
        m_ScreenWidth = newWidth;
        m_ScreenHeight = newHeight;
    }
private:
    void ShutdownBuffers();
    void RenderBuffers(ID3D11DeviceContext* deviceContext);

private:
    ID3D11Buffer* m_VertexBuffer {};
    ID3D11Buffer* m_IndexBuffer {};
    int m_ScreenWidth {}, m_ScreenHeight {};
    int m_VertexCount {}, m_IndexCount {};
    int m_MaxLength {};
    XMFLOAT4 m_pixelColor {};
};
