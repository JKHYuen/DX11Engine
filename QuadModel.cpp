#include "QuadModel.h"
#include "D3DClass.h"

QuadModel::QuadModel() {}
QuadModel::QuadModel(const QuadModel& other) {}
QuadModel::~QuadModel() {}

bool QuadModel::Initialize(ID3D11Device* device, float width, float height) {
    VertexType* vertices;
    unsigned long* indices;
    D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
    D3D11_SUBRESOURCE_DATA vertexData, indexData;
    HRESULT result;
    int i;

    // Set the number of vertices in the vertex array.
    m_vertexCount = 6;

    // Set the number of indices in the index array.
    m_indexCount = m_vertexCount;

    // Create the vertex array.
    vertices = new VertexType[m_vertexCount];

    // Create the index array.
    indices = new unsigned long[m_indexCount];

    // Load the vertex array with data.
    // First triangle.
    vertices[0].position = XMFLOAT3(-width, height, 0.0f);  // Top left.
    vertices[0].texture = XMFLOAT2(0.0f, 0.0f);

    vertices[1].position = XMFLOAT3(width, -height, 0.0f);  // Bottom right.
    vertices[1].texture = XMFLOAT2(1.0f, 1.0f);

    vertices[2].position = XMFLOAT3(-width, -height, 0.0f);  // Bottom left.
    vertices[2].texture = XMFLOAT2(0.0f, 1.0f);

    // Second triangle.
    vertices[3].position = XMFLOAT3(-width, height, 0.0f);  // Top left.
    vertices[3].texture = XMFLOAT2(0.0f, 0.0f);

    vertices[4].position = XMFLOAT3(width, height, 0.0f);  // Top right.
    vertices[4].texture = XMFLOAT2(1.0f, 0.0f);

    vertices[5].position = XMFLOAT3(width, -height, 0.0f);  // Bottom right.
    vertices[5].texture = XMFLOAT2(1.0f, 1.0f);

    // Load the index array with data.
    for(i = 0; i < m_indexCount; i++) {
        indices[i] = i;
    }

    // Set up the description of the vertex buffer.
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = sizeof(VertexType) * m_vertexCount;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.MiscFlags = 0;
    vertexBufferDesc.StructureByteStride = 0;

    // Give the subresource structure a pointer to the vertex data.
    vertexData.pSysMem = vertices;
    vertexData.SysMemPitch = 0;
    vertexData.SysMemSlicePitch = 0;

    // Now finally create the vertex buffer.
    result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
    if(FAILED(result)) {
        return false;
    }

    // Set up the description of the index buffer.
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(unsigned long) * m_indexCount;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.MiscFlags = 0;
    indexBufferDesc.StructureByteStride = 0;

    // Give the subresource structure a pointer to the index data.
    indexData.pSysMem = indices;
    indexData.SysMemPitch = 0;
    indexData.SysMemSlicePitch = 0;

    // Create the index buffer.
    result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
    if(FAILED(result)) {
        return false;
    }

    // Release the arrays now that the vertex and index buffers have been created and loaded.
    delete[] vertices;
    vertices = nullptr;

    delete[] indices;
    indices = nullptr;

    return true;
}

void QuadModel::Shutdown() {
    // Release the vertex and index buffers.
    ShutdownBuffers();
}

void QuadModel::Render(ID3D11DeviceContext* deviceContext) {
    unsigned int stride;
    unsigned int offset;

    // Set vertex buffer stride and offset.
    stride = sizeof(VertexType);
    offset = 0;

    // Set the vertex buffer to active in the input assembler so it can be rendered.
    deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

    // Set the index buffer to active in the input assembler so it can be rendered.
    deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    // Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

int QuadModel::GetIndexCount() {
    return m_indexCount;
}

void QuadModel::ShutdownBuffers() {
    // Release the index buffer.
    if(m_indexBuffer) {
        m_indexBuffer->Release();
        m_indexBuffer = nullptr;
    }

    // Release the vertex buffer.
    if(m_vertexBuffer) {
        m_vertexBuffer->Release();
        m_vertexBuffer = nullptr;
    }
}