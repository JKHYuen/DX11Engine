#include "Text.h"

bool Text::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, int screenWidth, int screenHeight, int maxLength, Font* Font, char* text, int positionX, int positionY, float red, float green, float blue) {
    // Store the screen width and height.
    m_ScreenWidth = screenWidth;
    m_ScreenHeight = screenHeight;

    // Store the maximum length of the sentence.
    m_MaxLength = maxLength;

    VertexType* vertices;
    unsigned long* indices;
    D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
    D3D11_SUBRESOURCE_DATA vertexData, indexData;
    HRESULT result;
    int i;

    // Set the vertex and index count.
    m_VertexCount = 6 * m_MaxLength;
    m_IndexCount = m_VertexCount;

    // Create the vertex array.
    vertices = new VertexType[m_VertexCount];

    // Create the index array.
    indices = new unsigned long[m_IndexCount];

    // Initialize vertex array to zeros at first.
    memset(vertices, 0, (sizeof(VertexType) * m_VertexCount));

    // Initialize the index array.
    for(i = 0; i < m_IndexCount; i++) {
        indices[i] = i;
    }

    // Set up the description of the dynamic vertex buffer.
    vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexBufferDesc.ByteWidth = sizeof(VertexType) * m_VertexCount;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    vertexBufferDesc.MiscFlags = 0;
    vertexBufferDesc.StructureByteStride = 0;

    // Give the subresource structure a pointer to the vertex data.
    vertexData.pSysMem = vertices;
    vertexData.SysMemPitch = 0;
    vertexData.SysMemSlicePitch = 0;

    // Create the vertex buffer.
    result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_VertexBuffer);
    if(FAILED(result)) {
        return false;
    }

    // Set up the description of the static index buffer.
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(unsigned long) * m_IndexCount;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.MiscFlags = 0;
    indexBufferDesc.StructureByteStride = 0;

    // Give the subresource structure a pointer to the index data.
    indexData.pSysMem = indices;
    indexData.SysMemPitch = 0;
    indexData.SysMemSlicePitch = 0;

    // Create the index buffer.
    result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_IndexBuffer);
    if(FAILED(result)) {
        return false;
    }

    // Release the vertex array as it is no longer needed.
    delete[] vertices;
    vertices = nullptr;

    // Release the index array as it is no longer needed.
    delete[] indices;
    indices = nullptr;

    // Now add the text data to the sentence buffers.
    result = UpdateText(deviceContext, Font, text, positionX, positionY, red, green, blue);
    if(FAILED(result)) {
        return false;
    }

    return true;
}

void Text::Shutdown() {
    // Release the vertex and index buffers.
    ShutdownBuffers();
}

void Text::Render(ID3D11DeviceContext* deviceContext) {
    // Put the vertex and index buffers on the graphics pipeline to prepare them for drawing.
    RenderBuffers(deviceContext);
}

int Text::GetIndexCount() {
    return m_IndexCount;
}

void Text::ShutdownBuffers() {
    // Release the index buffer.
    if(m_IndexBuffer) {
        m_IndexBuffer->Release();
        m_IndexBuffer = nullptr;
    }

    // Release the vertex buffer.
    if(m_VertexBuffer) {
        m_VertexBuffer->Release();
        m_VertexBuffer = nullptr;
    }
}

bool Text::UpdateText(ID3D11DeviceContext* deviceContext, Font* Font, char* text, int positionX, int positionY, float red, float green, float blue) {
    int numLetters;
    VertexType* vertices;
    float drawX, drawY;
    HRESULT result;
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    VertexType* verticesPtr;

    // Store the color of the sentence.
    m_pixelColor = XMFLOAT4(red, green, blue, 1.0f);

    // Get the number of letters in the sentence.
    numLetters = (int)strlen(text);

    // Check for possible buffer overflow.
    if(numLetters > m_MaxLength) {
        return false;
    }

    // Create the vertex array.
    vertices = new VertexType[m_VertexCount];

    // Initialize vertex array to zeros at first.
    memset(vertices, 0, (sizeof(VertexType) * m_VertexCount));

    // Calculate the X and Y pixel position on the screen to start drawing to.
    drawX = (float)(((m_ScreenWidth / 2) * -1) + positionX);
    drawY = (float)((m_ScreenHeight / 2) - positionY);

    // Use the font class to build the vertex array from the sentence text and sentence draw location.
    Font->BuildVertexArray((void*)vertices, text, drawX, drawY);

    // Lock the vertex buffer so it can be written to.
    result = deviceContext->Map(m_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if(FAILED(result)) {
        return false;
    }

    // Get a pointer to the data in the vertex buffer.
    verticesPtr = (VertexType*)mappedResource.pData;

    // Copy the data into the vertex buffer.
    memcpy(verticesPtr, (void*)vertices, (sizeof(VertexType) * m_VertexCount));

    // Unlock the vertex buffer.
    deviceContext->Unmap(m_VertexBuffer, 0);

    // Release the vertex array as it is no longer needed.
    delete[] vertices;
    vertices = nullptr;

    return true;
}

void Text::RenderBuffers(ID3D11DeviceContext* deviceContext) {
    unsigned int stride = sizeof(VertexType);
    unsigned int offset = 0;
    deviceContext->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
    deviceContext->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

XMFLOAT4 Text::GetPixelColor() {
    return m_pixelColor;
}