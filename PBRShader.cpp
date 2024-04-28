#include "PBRShader.h"
#include "ShaderUtil.h"

#include "DirectionalLight.h"
#include "Texture.h"
#include "Skybox.h"
#include "Camera.h"

bool PBRShader::Initialize(ID3D11Device* device, HWND hwnd) {
    /// Compile and initiailze shader objects
    const std::wstring vsFileName = L"./shaders/PBR.vs";
    const std::wstring psFileName = L"./shaders/PBR.ps";
    const std::wstring hsFileName = L"./shaders/PBR.hs";
    const std::wstring dsFileName = L"./shaders/PBR.ds";

    HRESULT result {};
    ID3D10Blob* errorMessage {};
    ID3D10Blob* vertexShaderBuffer {};
    ID3D10Blob* pixelShaderBuffer {};
    ID3D10Blob* domainShaderBuffer {};

    // Compile vertex shader code
    result = D3DCompileFromFile(vsFileName.c_str(), NULL, NULL, "PBRVertexShader", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &vertexShaderBuffer, &errorMessage);
    if(FAILED(result)) {
        if(errorMessage) {
            OutputShaderErrorMessage(errorMessage, hwnd, vsFileName.c_str());
        }
        else {
            MessageBox(hwnd, vsFileName.c_str(), L"Missing Shader File", MB_OK);
        }
        return false;
    }

    // Compile pixel shader code
    result = D3DCompileFromFile(psFileName.c_str(), NULL, NULL, "PBRPixelShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &pixelShaderBuffer, &errorMessage);
    if(FAILED(result)) {
        if(errorMessage) {
            OutputShaderErrorMessage(errorMessage, hwnd, psFileName.c_str());
        }
        else {
            MessageBox(hwnd, psFileName.c_str(), L"Missing Shader File", MB_OK);
        }
        return false;
    }

    // Compile domain shader code
    result = D3DCompileFromFile(dsFileName.c_str(), NULL, NULL, "PBRDomainShader", "ds_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &domainShaderBuffer, &errorMessage);
    if(FAILED(result)) {
        if(errorMessage) {
            OutputShaderErrorMessage(errorMessage, hwnd, dsFileName.c_str());
        }
        else {
            MessageBox(hwnd, dsFileName.c_str(), L"Missing Shader File", MB_OK);
        }
        return false;
    }

    // Create vertex shader
    result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &m_VertexShader);
    if(FAILED(result)) return false;

    // Create pixel shader
    result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &m_PixelShader);
    if(FAILED(result)) return false;

    // Create domain shader
    result = device->CreateDomainShader(domainShaderBuffer->GetBufferPointer(), domainShaderBuffer->GetBufferSize(), NULL, &m_DomainShader);
    if(FAILED(result)) return false;

    // Note: vertexShaderBuffer still needed for layout creation below
    pixelShaderBuffer->Release();
    pixelShaderBuffer = nullptr;

    domainShaderBuffer->Release();
    domainShaderBuffer = nullptr;

    // Initialize all hull shader variants
    D3D_SHADER_MACRO hullMacros[2] {{NULL, NULL}, {NULL, NULL}};
    for(int i = 0; i < TessellationMode::Num_TessellationModes; i++) {
        ID3D10Blob* hullShaderBuffer {};

        char buffer[3] {};
        sprintf_s(buffer, "%d", i);
        hullMacros[0] = {"TESS_MODE", buffer};

        result = D3DCompileFromFile(hsFileName.c_str(), hullMacros, NULL, "PBRHullShader", "hs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &hullShaderBuffer, &errorMessage);
        if(FAILED(result)) {
            if(errorMessage) {
                OutputShaderErrorMessage(errorMessage, hwnd, hsFileName.c_str());
            }
            else {
                MessageBox(hwnd, hsFileName.c_str(), L"Missing Shader File", MB_OK);
            }
            return false;
        }

        result = device->CreateHullShader(hullShaderBuffer->GetBufferPointer(), hullShaderBuffer->GetBufferSize(), NULL, &m_HullShaders[i]);
        if(FAILED(result)) return false;

        hullShaderBuffer->Release();
        hullShaderBuffer = nullptr;
    }

    // Create the vertex input layout description
    // Needs to match the VertexType stucture in the Model class and in shader
    D3D11_INPUT_ELEMENT_DESC polygonLayout[5] {};
    polygonLayout[0].SemanticName = "POSITION";
    polygonLayout[0].SemanticIndex = 0;
    polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    polygonLayout[0].InputSlot = 0;
    polygonLayout[0].AlignedByteOffset = 0;
    polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[0].InstanceDataStepRate = 0;

    polygonLayout[1].SemanticName = "TEXCOORD";
    polygonLayout[1].SemanticIndex = 0;
    polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    polygonLayout[1].InputSlot = 0;
    polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[1].InstanceDataStepRate = 0;

    polygonLayout[2].SemanticName = "NORMAL";
    polygonLayout[2].SemanticIndex = 0;
    polygonLayout[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    polygonLayout[2].InputSlot = 0;
    polygonLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    polygonLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[2].InstanceDataStepRate = 0;

    polygonLayout[3].SemanticName = "TANGENT";
    polygonLayout[3].SemanticIndex = 0;
    polygonLayout[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    polygonLayout[3].InputSlot = 0;
    polygonLayout[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    polygonLayout[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[3].InstanceDataStepRate = 0;

    polygonLayout[4].SemanticName = "BINORMAL";
    polygonLayout[4].SemanticIndex = 0;
    polygonLayout[4].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    polygonLayout[4].InputSlot = 0;
    polygonLayout[4].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    polygonLayout[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[4].InstanceDataStepRate = 0;

    unsigned int numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);
    result =
        device->CreateInputLayout(
            polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &m_Layout
        );

    if(FAILED(result)) {
        return false;
    }

    vertexShaderBuffer->Release();
    vertexShaderBuffer = nullptr;

    /// Create the texture sampler states
    D3D11_SAMPLER_DESC samplerDesc {};
    samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 16;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.BorderColor[0] = 0;
    samplerDesc.BorderColor[1] = 0;
    samplerDesc.BorderColor[2] = 0;
    samplerDesc.BorderColor[3] = 0;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    result = device->CreateSamplerState(&samplerDesc, &m_SampleStateWrap);
    if(FAILED(result)) {
        return false;
    }

    // For shadow map
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.BorderColor[0] = 1;
    samplerDesc.BorderColor[1] = 1;
    samplerDesc.BorderColor[2] = 1;
    samplerDesc.BorderColor[3] = 1;
    samplerDesc.MaxAnisotropy = 1;
    result = device->CreateSamplerState(&samplerDesc, &m_SampleStateBorder);
    if(FAILED(result)) {
        return false;
    }

    // For BRDF LUT
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    result = device->CreateSamplerState(&samplerDesc, &m_SampleStateClamp);
    if(FAILED(result)) {
        return false;
    }

    /// Setup matrix buffer
    D3D11_BUFFER_DESC matrixBufferDesc {};
    matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
    matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    matrixBufferDesc.MiscFlags = 0;
    matrixBufferDesc.StructureByteStride = 0;

    // Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
    result = device->CreateBuffer(&matrixBufferDesc, NULL, &m_MatrixBuffer);
    if(FAILED(result)) {
        return false;
    }

    //// Setup the description of the dynamic constant buffer that is in the pixel shader.
    //D3D11_BUFFER_DESC lightColorBufferDesc {};
    //lightColorBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    //lightColorBufferDesc.ByteWidth = sizeof(LightColorBufferType);
    //lightColorBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    //lightColorBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    //lightColorBufferDesc.MiscFlags = 0;
    //lightColorBufferDesc.StructureByteStride = 0;

    //// Create the constant buffer pointer so we can access the pixel shader constant buffer from within this class.
    //result = device->CreateBuffer(&lightColorBufferDesc, NULL, &m_lightColorBuffer);
    //if(FAILED(result)) {
    //    return false;
    //}

    //// Setup the description of the dynamic constant buffer that is in the vertex shader.
    //D3D11_BUFFER_DESC lightPositionBufferDesc {};
    //lightPositionBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    //lightPositionBufferDesc.ByteWidth = sizeof(LightPositionBufferType);
    //lightPositionBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    //lightPositionBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    //lightPositionBufferDesc.MiscFlags = 0;
    //lightPositionBufferDesc.StructureByteStride = 0;

    //// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
    //result = device->CreateBuffer(&lightPositionBufferDesc, NULL, &m_lightPositionBuffer);
    //if(FAILED(result)) {
    //    return false;
    //}
    
    /// Setup camera buffer
    D3D11_BUFFER_DESC cameraBufferDesc {};
    cameraBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    cameraBufferDesc.ByteWidth = sizeof(CameraBufferType);
    cameraBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cameraBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cameraBufferDesc.MiscFlags = 0;
    cameraBufferDesc.StructureByteStride = 0;

    // Create the camera constant buffer 
    result = device->CreateBuffer(&cameraBufferDesc, NULL, &m_CameraBuffer);
    if(FAILED(result)) {
        return false;
    }

    /// Setup light buffer
    D3D11_BUFFER_DESC lightBufferDesc {};
    lightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    lightBufferDesc.ByteWidth = sizeof(LightBufferType);
    lightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    lightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    lightBufferDesc.MiscFlags = 0;
    lightBufferDesc.StructureByteStride = 0;

    result = device->CreateBuffer(&lightBufferDesc, NULL, &m_LightBuffer);
    if(FAILED(result)) {
        return false;
    }

    /// Setup material param buffer
    D3D11_BUFFER_DESC materialParamBuffer {};
    materialParamBuffer.Usage = D3D11_USAGE_DYNAMIC;
    materialParamBuffer.ByteWidth = sizeof(MaterialParamBufferType);
    materialParamBuffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    materialParamBuffer.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    materialParamBuffer.MiscFlags = 0;
    materialParamBuffer.StructureByteStride = 0;

    result = device->CreateBuffer(&materialParamBuffer, NULL, &m_MaterialParamBuffer);
    if(FAILED(result)) {
        return false;
    }

    /// Setup tessellation buffers
    D3D11_BUFFER_DESC tessellationBufferDesc {};
    tessellationBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    tessellationBufferDesc.ByteWidth = sizeof(TessellationBufferType);
    tessellationBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    tessellationBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    tessellationBufferDesc.MiscFlags = 0;
    tessellationBufferDesc.StructureByteStride = 0;

    result = device->CreateBuffer(&tessellationBufferDesc, NULL, &m_TessellationBuffer);
    if(FAILED(result)) {
        return false;
    }

    return true;
}

bool PBRShader::InitializeHullShaders(ID3D11Device* device, const std::wstring& hsFileName, HWND hwnd) {
    HRESULT result {};
    ID3D10Blob* errorMessage {};
    D3D_SHADER_MACRO hullMacros[2] {{NULL, NULL}, {NULL, NULL}};
    for(int i = 0; i < TessellationMode::Num_TessellationModes; i++) {
        ID3D10Blob* hullShaderBuffer {};

        char buffer[3] {};
        sprintf_s(buffer, "%d", i);
        hullMacros[0] = {"TESS_MODE", buffer};

        result = D3DCompileFromFile(hsFileName.c_str(), hullMacros, NULL, "PBRHullShader", "hs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &hullShaderBuffer, &errorMessage);
        if(FAILED(result)) {
            if(errorMessage) {
                OutputShaderErrorMessage(errorMessage, hwnd, hsFileName.c_str());
            }
            else {
                MessageBox(hwnd, hsFileName.c_str(), L"Missing Shader File", MB_OK);
            }
            return false;
        }

        result = device->CreateHullShader(hullShaderBuffer->GetBufferPointer(), hullShaderBuffer->GetBufferSize(), NULL, &m_HullShaders[i]);
        if(FAILED(result)) return false;

        hullShaderBuffer->Release();
        hullShaderBuffer = nullptr;
    }
    return true;
}

bool PBRShader::Render(ID3D11DeviceContext* deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX projectionMatrix, const std::vector<Texture*> materialTextures, ID3D11ShaderResourceView* shadowMap, Skybox* skybox, DirectionalLight* light, Camera* camera, const std::array<XMFLOAT4, 6>& cullFrustum, float time, const GameObject::GameObjectData& gameObjectData) {
    HRESULT result;
    //LightPositionBufferType* dataPtr2;
    //LightColorBufferType* dataPtr3;

    XMMATRIX viewMatrix {};
    camera->GetViewMatrix(viewMatrix);

    LightBufferType* lightDataPtr;
    CameraBufferType* cameraDataPtr;

    MaterialParamBufferType* materialParamDataPtr;
    TessellationBufferType* tessellationDataPtr;

    /// Domain Shader Marix cbuffer 
    worldMatrix = XMMatrixTranspose(worldMatrix);
    viewMatrix = XMMatrixTranspose(viewMatrix);
    projectionMatrix = XMMatrixTranspose(projectionMatrix);

    XMMATRIX lightViewMatrix {};
    XMMATRIX lightOrthoMatrix {};
    light->GetViewMatrix(lightViewMatrix);
    light->GetOrthoMatrix(lightOrthoMatrix);

    lightViewMatrix = XMMatrixTranspose(lightViewMatrix);
    lightOrthoMatrix = XMMatrixTranspose(lightOrthoMatrix);

    D3D11_MAPPED_SUBRESOURCE mappedResource {};
    result = deviceContext->Map(m_MatrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if(FAILED(result)) {
        return false;
    }

    MatrixBufferType* matrixDataPtr = (MatrixBufferType*)mappedResource.pData;

    matrixDataPtr->world = worldMatrix;
    matrixDataPtr->view = viewMatrix;
    matrixDataPtr->projection = projectionMatrix;
    matrixDataPtr->lightView = lightViewMatrix;
    matrixDataPtr->lightProjection = lightOrthoMatrix;

    deviceContext->Unmap(m_MatrixBuffer, 0);
    deviceContext->DSSetConstantBuffers(0, 1, &m_MatrixBuffer);

    /// Domain Shader camera cbuffer
    result = deviceContext->Map(m_CameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if(FAILED(result)) {
        return false;
    }

    cameraDataPtr = (CameraBufferType*)mappedResource.pData;

    cameraDataPtr->cameraPosition = camera->GetPosition();
    cameraDataPtr->displacementHeightScale = gameObjectData.vertexDisplacementMapScale;
    cameraDataPtr->uvScale = gameObjectData.uvScale;
    cameraDataPtr->padding = {};

    deviceContext->Unmap(m_CameraBuffer, 0);
    deviceContext->DSSetConstantBuffers(1, 1, &m_CameraBuffer);

    ///// VS point light pos buffer
    //// Lock the light position constant buffer so it can be written to.
    //result = deviceContext->Map(m_lightPositionBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    //if(FAILED(result)) {
    //    return false;
    //}

    //// Get a pointer to the data in the constant buffer.
    //dataPtr2 = (LightPositionBufferType*)mappedResource.pData;

    //// Copy the light position variables into the constant buffer.
    //dataPtr2->lightPosition[0] = lightPosition[0];
    //dataPtr2->lightPosition[1] = lightPosition[1];
    //dataPtr2->lightPosition[2] = lightPosition[2];
    //dataPtr2->lightPosition[3] = lightPosition[3];

    //// Unlock the constant buffer.
    //deviceContext->Unmap(m_lightPositionBuffer, 0);

    //// Set the position of the constant buffer in the vertex shader.
    //bufferNumber = 1;

    //// Finally set the constant buffer in the vertex shader with the updated values.
    //deviceContext->VSSetConstantBuffers(bufferNumber, 1, &m_lightPositionBuffer);

    /// Bind Pixel Shader textures
    /// Order of materialTextures array: albedoMap, normalMap, metallicMap, roughnessMap, aoMap, heightMap
    ID3D11ShaderResourceView* pTempSRV;
    for(int i = 0; i < 6; i++) {
        pTempSRV = materialTextures[i]->GetTextureSRV();
        deviceContext->PSSetShaderResources(i, 1, &pTempSRV);
    }

    // for indirect lighting
    deviceContext->PSSetShaderResources(6, 1, &shadowMap);
    ID3D11ShaderResourceView* pIrradianceMap = skybox->GetIrradianceMapSRV();
    ID3D11ShaderResourceView* pPrefilteredMap = skybox->GetPrefilteredMapSRV();
    ID3D11ShaderResourceView* pBRDFLut = skybox->GetPrecomputedBRDFSRV();
    deviceContext->PSSetShaderResources(7, 1, &pIrradianceMap);
    deviceContext->PSSetShaderResources(8, 1, &pPrefilteredMap);
    deviceContext->PSSetShaderResources(9, 1, &pBRDFLut);

    /// Bind Domain Shader Textures
    pTempSRV = materialTextures[5]->GetTextureSRV(); // height map
    deviceContext->DSSetShaderResources(0, 1, &pTempSRV);

    /// Pixel Shader Light cbuffer
    result = deviceContext->Map(m_LightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if(FAILED(result)) {
        return false;
    }

    lightDataPtr = (LightBufferType*)mappedResource.pData;

    lightDataPtr->diffuseColor = light->GetDirectionalColor();
    lightDataPtr->lightDirection = light->GetDirection();
    lightDataPtr->time = time;

    deviceContext->Unmap(m_LightBuffer, 0);
    deviceContext->PSSetConstantBuffers(0, 1, &m_LightBuffer);

    /// Pixel Shader Material Param cbuffer
    result = deviceContext->Map(m_MaterialParamBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if(FAILED(result)) {
        return false;
    }

    materialParamDataPtr = (MaterialParamBufferType*)mappedResource.pData;

    materialParamDataPtr->parallaxHeightScale = gameObjectData.parallaxMapHeightScale;
    materialParamDataPtr->minRoughness = gameObjectData.minRoughness;
    materialParamDataPtr->padding = {};

    materialParamDataPtr->useParallaxShadow = gameObjectData.useParallaxShadow ? 1.0f : 0.0f;
    materialParamDataPtr->minParallaxLayers = (float)gameObjectData.minParallaxLayers;
    materialParamDataPtr->maxParallaxLayers = (float)gameObjectData.maxParallaxLayers;
    materialParamDataPtr->shadowBias = light->GetShadowBias();

    deviceContext->Unmap(m_MaterialParamBuffer, 0);
    deviceContext->PSSetConstantBuffers(1, 1, &m_MaterialParamBuffer);

    /// Hull Shader cbuffer
    result = deviceContext->Map(m_TessellationBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if(FAILED(result)) {
        return false;
    }

    tessellationDataPtr = (TessellationBufferType*)mappedResource.pData;

    float tessellationFactor {};
    // tessellationMode: kDisabledTess = 0, kUniformTess = 1, kEdgeTess = 2,
    if(gameObjectData.tessellationMode == 1) {
        tessellationFactor = gameObjectData.uniformTessellationFactor;
    }
    else if(gameObjectData.tessellationMode == 2) {
        tessellationFactor = gameObjectData.edgeTessellationLength;
    }
    tessellationDataPtr->tessellationFactor = tessellationFactor;

    tessellationDataPtr->cameraPosition = camera->GetPosition();
    tessellationDataPtr->world = worldMatrix;

    // Ignore near and far planes (see Camera.cpp for plane order)
    tessellationDataPtr->cullPlanes[0] = cullFrustum[2];
    tessellationDataPtr->cullPlanes[1] = cullFrustum[3];
    tessellationDataPtr->cullPlanes[2] = cullFrustum[4];
    tessellationDataPtr->cullPlanes[3] = cullFrustum[5];

    tessellationDataPtr->cullBias = gameObjectData.vertexDisplacementMapScale;
    tessellationDataPtr->screenDimensions = XMFLOAT2((float)GetSystemMetrics(SM_CXSCREEN), (float)GetSystemMetrics(SM_CYSCREEN));
    tessellationDataPtr->padding = {};

    deviceContext->Unmap(m_TessellationBuffer, 0);
    deviceContext->HSSetConstantBuffers(0, 1, &m_TessellationBuffer);

    ///// PS point light color buffer
    //// Lock the light color constant buffer so it can be written to.
    //result = deviceContext->Map(m_lightColorBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    //if(FAILED(result)) {
    //    return false;
    //}

    //// Get a pointer to the data in the constant buffer.
    //dataPtr3 = (LightColorBufferType*)mappedResource.pData;

    //// Copy the light color variables into the constant buffer.
    //dataPtr3->diffuseColor[0] = diffuseColor[0];
    //dataPtr3->diffuseColor[1] = diffuseColor[1];
    //dataPtr3->diffuseColor[2] = diffuseColor[2];
    //dataPtr3->diffuseColor[3] = diffuseColor[3];

    //// Unlock the constant buffer.
    //deviceContext->Unmap(m_lightColorBuffer, 0);

    //// Set the position of the constant buffer in the pixel shader.
    //bufferNumber = 0;

    //// Finally set the constant buffer in the pixel shader with the updated values.
    //deviceContext->PSSetConstantBuffers(bufferNumber, 1, &m_lightColorBuffer);

    /// Render
    deviceContext->IASetInputLayout(m_Layout);

    deviceContext->VSSetShader(m_VertexShader, NULL, 0);
    deviceContext->HSSetShader(m_HullShaders[gameObjectData.tessellationMode], NULL, 0);
    deviceContext->DSSetShader(m_DomainShader, NULL, 0);
    deviceContext->PSSetShader(m_PixelShader, NULL, 0);

    deviceContext->PSSetSamplers(0, 1, &m_SampleStateWrap);
    deviceContext->PSSetSamplers(1, 1, &m_SampleStateBorder);
    deviceContext->PSSetSamplers(2, 1, &m_SampleStateClamp);

    deviceContext->DSSetSamplers(0, 1, &m_SampleStateWrap);

    deviceContext->DrawIndexed(indexCount, 0, 0);

    return true;
}

void PBRShader::Shutdown() {
    if(m_LightBuffer) {
        m_LightBuffer->Release();
        m_LightBuffer = nullptr;
    }

    if(m_CameraBuffer) {
        m_CameraBuffer->Release();
        m_CameraBuffer = nullptr;
    }

    if(m_MaterialParamBuffer) {
        m_MaterialParamBuffer->Release();
        m_MaterialParamBuffer = nullptr;
    }

    //if(m_lightColorBuffer) {
    //    m_lightColorBuffer->Release();
    //    m_lightColorBuffer = 0;
    //}

    //if(m_lightPositionBuffer) {
    //    m_lightPositionBuffer->Release();
    //    m_lightPositionBuffer = 0;
    //}

    if(m_MatrixBuffer) {
        m_MatrixBuffer->Release();
        m_MatrixBuffer = nullptr;
    }

    if(m_TessellationBuffer) {
        m_TessellationBuffer->Release();
        m_TessellationBuffer = nullptr;
    }

    if(m_SampleStateWrap) {
        m_SampleStateWrap->Release();
        m_SampleStateWrap = nullptr;
    }

    if(m_SampleStateBorder) {
        m_SampleStateBorder->Release();
        m_SampleStateBorder = nullptr;
    }

    if(m_SampleStateClamp) {
        m_SampleStateClamp->Release();
        m_SampleStateClamp = nullptr;
    }

    if(m_Layout) {
        m_Layout->Release();
        m_Layout = nullptr;
    }

    if(m_PixelShader) {
        m_PixelShader->Release();
        m_PixelShader = nullptr;
    }

    if(m_VertexShader) {
        m_VertexShader->Release();
        m_VertexShader = nullptr;
    }

    for(int i = 0; i < m_HullShaders.size(); i++) {
        m_HullShaders[i]->Release();
        m_HullShaders[i] = nullptr;
    }

    if(m_DomainShader) {
        m_DomainShader->Release();
        m_DomainShader = nullptr;
    }
}
