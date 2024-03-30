#include "GameObject.h"
#include "PBRShader.h"
#include "DepthShader.h"
#include "Texture.h"
#include "Model.h"
#include "DirectionalLight.h"

bool GameObject::Initialize(PBRShader* pbrShaderInstance, DepthShader* depthShaderInstance, const std::vector<Texture*>& textureResources, Model* model) {
	m_MaterialTextures = textureResources;
	m_ModelInstance = model;

	/// Initialize Shaders
	m_PBRShaderInstance = pbrShaderInstance;
	m_DepthShaderInstance = depthShaderInstance;

	return true;
}

bool GameObject::RenderToDepth(ID3D11DeviceContext* deviceContext, DirectionalLight* light, float time){
	XMMATRIX srtMatrix = XMMatrixMultiply(XMMatrixMultiply(
		XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z),
		XMMatrixRotationY(time * m_RotationYSpeed)),
		XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z)
	);

	m_ModelInstance->Render(deviceContext);

	XMMATRIX lightView {};
	XMMATRIX lightProjection {};
	light->GetViewMatrix(lightView);
	light->GetOrthoMatrix(lightProjection);
	m_DepthShaderInstance->Render(deviceContext, m_ModelInstance->GetIndexCount(), srtMatrix, lightView, lightProjection);
	return true;
}

// TODO: use CubeMapObject as parameter
bool GameObject::Render(ID3D11DeviceContext* deviceContext, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ID3D11ShaderResourceView* shadowMap, ID3D11ShaderResourceView* irradianceMap, ID3D11ShaderResourceView* prefilteredMap, ID3D11ShaderResourceView* BRDFLut, DirectionalLight* light, XMFLOAT3 cameraPos, float time) {
	XMMATRIX srtMatrix = XMMatrixMultiply(XMMatrixMultiply(
		XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z),
		XMMatrixRotationY(time * m_RotationYSpeed)),
		XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z)
	);

	m_ModelInstance->Render(deviceContext);
	// Render the model using the light shader.
	return m_PBRShaderInstance->Render(deviceContext, m_ModelInstance->GetIndexCount(), srtMatrix, viewMatrix, projectionMatrix, m_MaterialTextures, shadowMap, irradianceMap, prefilteredMap, BRDFLut, light, cameraPos, time, m_UVScale, m_DisplacementHeightScale, m_ParallaxHeightScale);
}

void GameObject::Shutdown() {
	//if(m_Model) {
	//	m_Model->Shutdown();
	//	delete m_Model;
	//	m_Model = nullptr;
	//}
}
