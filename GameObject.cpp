#include <vector>

#include "GameObject.h"
#include "PBRShaderClass.h"
#include "DepthShaderClass.h"
#include "ModelClass.h"
#include "LightClass.h"
#include "RenderTextureClass.h"

bool GameObject::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND windowHandle, const std::string& modelName, const std::string& textureName) {
	bool result;

	// TODO: convert to wstring
	const std::vector<std::string> textureFileNames {
		"../DX11Engine/data/" + textureName + "/" + textureName + "_albedo.tga",
		"../DX11Engine/data/" + textureName + "/" + textureName + "_normal.tga",
		"../DX11Engine/data/" + textureName + "/" + textureName + "_metallic.tga",
		"../DX11Engine/data/" + textureName + "/" + textureName + "_roughness.tga",
		"../DX11Engine/data/" + textureName + "/" + textureName + "_ao.tga",
		"../DX11Engine/data/" + textureName + "/" + textureName + "_height.tga"
	};

	/*const std::vector<std::string> textureFileNames {
	"../DX11Engine/data/" + textureName + "/png/" + textureName + "_albedo.png",
	"../DX11Engine/data/" + textureName + "/png/" + textureName + "_normal.png",
	"../DX11Engine/data/" + textureName + "/png/" + textureName + "_metallic.png",
	"../DX11Engine/data/" + textureName + "/png/" + textureName + "_roughness.png",
	"../DX11Engine/data/" + textureName + "/png/" + textureName + "_ao.png",
	"../DX11Engine/data/" + textureName + "/png/" + textureName + "_height.tga"
	};*/

	// Create and initialize the model object.
	m_Model = new ModelClass();
	result = m_Model->Initialize(device, deviceContext, 
		"../DX11Engine/data/" + modelName + ".txt",
		textureFileNames
	);

	if(!result) {
		return false;
	}

	m_DepthShader = new DepthShaderClass();
	result = m_DepthShader->Initialize(device, windowHandle);
	if(!result) {
		MessageBox(windowHandle, L"Could not initialize the Depth shader object.", L"Error", MB_OK);
		return false;
	}

	// Create and initialize the light shader object.
	m_PBRShader = new PBRShaderClass();
	result = m_PBRShader->Initialize(device, windowHandle);
	if(!result) {
		MessageBox(windowHandle, L"Could not initialize the light shader object.", L"Error", MB_OK);
		return false;
	}

	return true;
}

bool GameObject::RenderToDepth(ID3D11DeviceContext* deviceContext, LightClass* light, float time){
	XMMATRIX srtMatrix = XMMatrixMultiply(XMMatrixMultiply(
		XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z),
		XMMatrixRotationY(time * m_RotationYSpeed)),
		XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z)
	);

	m_Model->Render(deviceContext);

	XMMATRIX lightView {};
	XMMATRIX lightProjection {};
	light->GetViewMatrix(lightView);
	light->GetOrthoMatrix(lightProjection);
	m_DepthShader->Render(deviceContext, m_Model->GetIndexCount(), srtMatrix, lightView, lightProjection);
	return true;
}

bool GameObject::Render(ID3D11DeviceContext* deviceContext, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ID3D11ShaderResourceView* shadowMap, ID3D11ShaderResourceView* irradianceMap, LightClass* light, XMFLOAT3 cameraPos, float time) {
	XMMATRIX srtMatrix = XMMatrixMultiply(XMMatrixMultiply(
		XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z),
		XMMatrixRotationY(time * m_RotationYSpeed)),
		XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z)
	);

	m_Model->Render(deviceContext);
	// Render the model using the light shader.
	return m_PBRShader->Render(deviceContext, m_Model->GetIndexCount(), srtMatrix, viewMatrix, projectionMatrix, m_Model->GetTexture(0), m_Model->GetTexture(1), m_Model->GetTexture(2), m_Model->GetTexture(3), m_Model->GetTexture(4), m_Model->GetTexture(5), shadowMap, irradianceMap, light, cameraPos, time, m_UVScale, m_HeightMapScale);
}

void GameObject::Shutdown() {
	if(m_Model) {
		m_Model->Shutdown();
		delete m_Model;
		m_Model = nullptr;
	}

	if(m_PBRShader) {
		m_PBRShader->Shutdown();
		delete m_PBRShader;
		m_PBRShader = nullptr;
	}
}
