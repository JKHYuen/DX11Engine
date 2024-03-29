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

private:
    void ShutdownBuffers();
    void RenderBuffers(ID3D11DeviceContext* deviceContext);

private:
    ID3D11Buffer* m_vertexBuffer {}, * m_indexBuffer {};
    int m_screenWidth {}, m_screenHeight {}, m_maxLength {}, m_vertexCount {}, m_indexCount {};
    XMFLOAT4 m_pixelColor {};
};
