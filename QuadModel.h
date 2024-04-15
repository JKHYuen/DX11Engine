#pragma once

#include <d3d11.h>
#include <directxmath.h>
using namespace DirectX;

class QuadModel {
private:
    struct VertexType {
        XMFLOAT3 position;
        XMFLOAT2 texture;
    };

public:
    QuadModel() {}
    QuadModel(const QuadModel&) {}
    ~QuadModel() {}

    bool Initialize(ID3D11Device*, float, float);
    void Shutdown();
    void Render(ID3D11DeviceContext*) const;

    int GetIndexCount();

private:
    ID3D11Buffer* m_VertexBuffer {};
    ID3D11Buffer* m_IndexBuffer {};
    int m_VertexCount {};
    int m_IndexCount {};
};