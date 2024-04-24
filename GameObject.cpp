#include "GameObject.h"
#include "PBRShader.h"
#include "DepthShader.h"
#include "Texture.h"
#include "Model.h"
#include "DirectionalLight.h"
#include "Camera.h"

#include <iostream>

// Note: "instances" passed as parameters are cleaned up in scene class
bool GameObject::Initialize(PBRShader* pbrShaderInstance, DepthShader* depthShaderInstance, const std::vector<Texture*>& textureResources, Model* model, const GameObjectData& initialGameObjectData) {
	m_MaterialTextures = textureResources;
	m_ModelInstance = model;
	m_PBRShaderInstance = pbrShaderInstance;
	m_DepthShaderInstance = depthShaderInstance;

	m_GameObjectData = initialGameObjectData;
	
	return true;
}

bool GameObject::RenderToDepth(ID3D11DeviceContext* deviceContext, DirectionalLight* light, float time){
	if(!mb_IsEnabled) {
		return true;
	}

	XMMATRIX srtMatrix = XMMatrixMultiply(XMMatrixMultiply(
		XMMatrixScaling(m_GameObjectData.scale.x, m_GameObjectData.scale.y, m_GameObjectData.scale.z),
		XMMatrixRotationY(time * m_GameObjectData.yRotSpeed)),
		XMMatrixTranslation(m_GameObjectData.position.x, m_GameObjectData.position.y, m_GameObjectData.position.z)
	);

	m_ModelInstance->Render(deviceContext, true);

	XMMATRIX lightView {};
	XMMATRIX lightProjection {};
	light->GetViewMatrix(lightView);
	light->GetOrthoMatrix(lightProjection);
	m_DepthShaderInstance->Render(deviceContext, m_ModelInstance->GetIndexCount(), srtMatrix, lightView, lightProjection, m_MaterialTextures[5], m_GameObjectData);
	return true;
}

// TODO: use CubeMapObject as parameter?
bool GameObject::Render(ID3D11DeviceContext* deviceContext, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ID3D11ShaderResourceView* shadowMap, ID3D11ShaderResourceView* irradianceMap, ID3D11ShaderResourceView* prefilteredMap, ID3D11ShaderResourceView* BRDFLut, DirectionalLight* light, Camera* camera, float time) {
	if(!mb_IsEnabled) {
		return true;
	}

	/// Frustrum visibililty check
	/// WARNING: extents (bounding box) currently don't take rotation into account
	XMFLOAT3 modelExtents = m_ModelInstance->GetExtents();
	modelExtents.x *= m_GameObjectData.scale.x;
	modelExtents.y *= m_GameObjectData.scale.y;
	modelExtents.z *= m_GameObjectData.scale.z;

	// NOTE: make sure vertexDisplacementMapScale use matches shader (i.e. not shifted 0.5 or something)
	if(!camera->CheckRectangleInFrustrum(m_GameObjectData.position.x, m_GameObjectData.position.y, m_GameObjectData.position.z, modelExtents.x, modelExtents.y, modelExtents.z, -m_GameObjectData.vertexDisplacementMapScale)) {
		return true;
	}

	/// Render
	XMMATRIX srtMatrix = XMMatrixMultiply(XMMatrixMultiply(
		XMMatrixScaling(m_GameObjectData.scale.x, m_GameObjectData.scale.y, m_GameObjectData.scale.z),
		XMMatrixRotationY(time * m_GameObjectData.yRotSpeed)),
		XMMatrixTranslation(m_GameObjectData.position.x, m_GameObjectData.position.y, m_GameObjectData.position.z)
	);

	m_ModelInstance->Render(deviceContext, true);
	return m_PBRShaderInstance->Render(deviceContext, m_ModelInstance->GetIndexCount(), srtMatrix, viewMatrix, projectionMatrix, m_MaterialTextures, shadowMap, irradianceMap, prefilteredMap, BRDFLut, light, camera->GetPosition(), time, m_GameObjectData);
}
