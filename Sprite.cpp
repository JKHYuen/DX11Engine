#include "Sprite.h"

Sprite::Sprite() {
    m_vertexBuffer = nullptr;
    m_indexBuffer  = nullptr;
    m_Textures     = nullptr;
}

Sprite::Sprite(const Sprite& other) {}
Sprite::~Sprite() {}

bool Sprite::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, int screenWidth, int screenHeight, char* textureFilename, int renderX, int renderY) {
    bool result;

    // Store the screen size.
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    // Store where the bitmap should be rendered to.
    m_renderX = renderX;
    m_renderY = renderY;

    // Initialize the frame time for this sprite object.
    m_frameTime = 0;

    // Initialize the vertex and index buffer that hold the geometry for the bitmap quad.
    result = InitializeBuffers(device);
    if(!result) {
        return false;
    }

    // Load the texture for this bitmap.
    result = LoadTextures(device, deviceContext, textureFilename);
    if(!result) {
        return false;
    }

    return true;
}

void Sprite::Shutdown() {
    // Release the bitmap texture.
    ReleaseTexture();

    // Release the vertex and index buffers.
    ShutdownBuffers();

    return;
}
bool Sprite::Render(ID3D11DeviceContext* deviceContext) {
    bool result;

    // Update the buffers if the position of the bitmap has changed from its original position.
    result = UpdateBuffers(deviceContext);
    if(!result) {
        return false;
    }

    // Put the vertex and index buffers on the graphics pipeline to prepare them for drawing.
    RenderBuffers(deviceContext);

    return true;
}

void Sprite::Update(float frameTime) {
    // Increment the frame time each frame.
    m_frameTime += frameTime;

    // Check if the frame time has reached the cycle time.
    if(m_frameTime >= m_cycleTime) {
        // If it has then reset the frame time and cycle to the next sprite in the texture array.
        m_frameTime -= m_cycleTime;

        m_currentTexture++;

        // If we are at the last sprite texture then go back to the beginning of the texture array to the first texture again.
        if(m_currentTexture == m_textureCount) {
            m_currentTexture = 0;
        }
    }

    return;
}

int Sprite::GetIndexCount() {
    return m_indexCount;
}

ID3D11ShaderResourceView* Sprite::GetTexture() {
    return m_Textures[m_currentTexture].GetTextureSRV();
}

bool Sprite::InitializeBuffers(ID3D11Device* device) {
    VertexType* vertices;
    unsigned long* indices;
    D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
    D3D11_SUBRESOURCE_DATA vertexData, indexData;
    HRESULT result;
    int i;

    // Initialize the previous rendering position to negative one.
    m_prevPosX = -1;
    m_prevPosY = -1;

    // Set the number of vertices in the vertex array.
    m_vertexCount = 6;

    // Set the number of indices in the index array.
    m_indexCount = m_vertexCount;

    // Create the vertex array.
    vertices = new VertexType[m_vertexCount];

    // Create the index array.
    indices = new unsigned long[m_indexCount];

    // Initialize vertex array to zeros at first.
    memset(vertices, 0, (sizeof(VertexType) * m_vertexCount));

    // Load the index array with data.
    for(i = 0; i < m_indexCount; i++) {
        indices[i] = i;
    }

    // Set up the description of the dynamic vertex buffer.
    vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexBufferDesc.ByteWidth = sizeof(VertexType) * m_vertexCount;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
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

void Sprite::ShutdownBuffers() {
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

bool Sprite::UpdateBuffers(ID3D11DeviceContext* deviceContent) {
    float left, right, top, bottom;
    VertexType* vertices;
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    VertexType* dataPtr;
    HRESULT result;

    // If the position we are rendering this bitmap to hasn't changed then don't update the vertex buffer.
    if((m_prevPosX == m_renderX) && (m_prevPosY == m_renderY)) {
        return true;
    }

    // If the rendering location has changed then store the new position and update the vertex buffer.
    m_prevPosX = m_renderX;
    m_prevPosY = m_renderY;

    // Create the vertex array.
    vertices = new VertexType[m_vertexCount];

    // Calculate the screen coordinates of the left side of the bitmap.
    left = (float)((m_screenWidth / 2) * -1) + (float)m_renderX;

    // Calculate the screen coordinates of the right side of the bitmap.
    right = left + (float)m_bitmapWidth;

    // Calculate the screen coordinates of the top of the bitmap.
    top = (float)(m_screenHeight / 2) - (float)m_renderY;

    // Calculate the screen coordinates of the bottom of the bitmap.
    bottom = top - (float)m_bitmapHeight;

    // Load the vertex array with data.
    // First triangle.
    vertices[0].position = XMFLOAT3(left, top, 0.0f);  // Top left.
    vertices[0].texture = XMFLOAT2(0.0f, 0.0f);

    vertices[1].position = XMFLOAT3(right, bottom, 0.0f);  // Bottom right.
    vertices[1].texture = XMFLOAT2(1.0f, 1.0f);

    vertices[2].position = XMFLOAT3(left, bottom, 0.0f);  // Bottom left.
    vertices[2].texture = XMFLOAT2(0.0f, 1.0f);

    // Second triangle.
    vertices[3].position = XMFLOAT3(left, top, 0.0f);  // Top left.
    vertices[3].texture = XMFLOAT2(0.0f, 0.0f);

    vertices[4].position = XMFLOAT3(right, top, 0.0f);  // Top right.
    vertices[4].texture = XMFLOAT2(1.0f, 0.0f);

    vertices[5].position = XMFLOAT3(right, bottom, 0.0f);  // Bottom right.
    vertices[5].texture = XMFLOAT2(1.0f, 1.0f);

    // Lock the vertex buffer.
    result = deviceContent->Map(m_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if(FAILED(result)) {
        return false;
    }

    // Get a pointer to the data in the constant buffer.
    dataPtr = (VertexType*)mappedResource.pData;

    // Copy the data into the vertex buffer.
    memcpy(dataPtr, (void*)vertices, (sizeof(VertexType) * m_vertexCount));

    // Unlock the vertex buffer.
    deviceContent->Unmap(m_vertexBuffer, 0);

    // Release the pointer reference.
    dataPtr = nullptr;

    // Release the vertex array as it is no longer needed.
    delete[] vertices;
    vertices = nullptr;

    return true;
}

void Sprite::RenderBuffers(ID3D11DeviceContext* deviceContext) {
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

    return;
}

bool Sprite::LoadTextures(ID3D11Device* device, ID3D11DeviceContext* deviceContext, char* filename) {
    char textureFilename[128];
    std::ifstream fin;
    int i, j;
    char input;
    bool result;

    // Open the sprite info data file.
    fin.open(filename);
    if(fin.fail()) {
        return false;
    }

    // Read in the number of textures.
    fin >> m_textureCount;

    // Create and initialize the texture object.
    m_Textures = new Texture[m_textureCount];

    // Read to start of next line.
    fin.get(input);

    // Read in each texture file name.
    for(i = 0; i < m_textureCount; i++) {
        j = 0;
        fin.get(input);
        while(input != '\n') {
            textureFilename[j] = input;
            j++;
            fin.get(input);
        }
        textureFilename[j] = '\0';

        // Once you have the filename then load the texture in the texture array.
        result = m_Textures[i].Initialize(device, deviceContext, textureFilename);
        if(!result) {
            return false;
        }
    }

    // Read in the cycle time.
    fin >> m_cycleTime;

    // Convert the integer milliseconds to float representation.
    m_cycleTime = m_cycleTime * 0.001f;

    // Close the file.
    fin.close();

    // Get the dimensions of the first texture and use that as the dimensions of the 2D sprite images.
    m_bitmapWidth = m_Textures[0].GetWidth();
    m_bitmapHeight = m_Textures[0].GetHeight();

    // Set the starting texture in the cycle to be the first one in the list.
    m_currentTexture = 0;

    return true;
}

void Sprite::ReleaseTexture() {
    // Release the texture object.
    if(m_Textures) {
        for(int i = 0; i < m_textureCount; i++) {
            m_Textures[i].Shutdown();
        }
        delete[] m_Textures;
        m_Textures = nullptr;
    }
}

void Sprite::SetRenderLocation(int x, int y) {
    m_renderX = x;
    m_renderY = y;
}
