#pragma once

#include "d3dclass.h"
class DisplayPlaneClass {
private:
    struct VertexType {
        XMFLOAT3 position;
        XMFLOAT2 texture;
    };

public:
    DisplayPlaneClass();
    DisplayPlaneClass(const DisplayPlaneClass&);
    ~DisplayPlaneClass();

    bool Initialize(ID3D11Device*, float, float);
    void Shutdown();
    void Render(ID3D11DeviceContext*);

    int GetIndexCount();

private:
    bool InitializeBuffers(ID3D11Device*, float, float);
    void ShutdownBuffers();

private:
    ID3D11Buffer* m_vertexBuffer {};
    ID3D11Buffer* m_indexBuffer {};
    int m_vertexCount {};
    int m_indexCount {};
};