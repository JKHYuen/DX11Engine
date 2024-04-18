#pragma once
#include "GameObject.h"

#include <d3d11.h>
#include <directxmath.h>
using namespace DirectX;

class Texture;

class DepthShader {
private:
	struct MatrixBufferType {
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
	};

	struct DepthMaterialBufferType {
		float heightMapScale;
		float uvScale;
		XMFLOAT2 padding;
	};

	struct TessellationBufferType {
		float tessellationFactor;
		XMFLOAT3 padding;
	};

public:
	DepthShader();
	DepthShader(const DepthShader&);
	~DepthShader();

	bool Initialize(ID3D11Device*, HWND);
	void Shutdown();
	bool Render(ID3D11DeviceContext* deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix,
		XMMATRIX projectionMatrix, Texture* heightMap, const GameObject::GameObjectData& gameObjectData);

private:
	ID3D11VertexShader* m_VertexShader {};
	ID3D11PixelShader*  m_PixelShader {};
	ID3D11HullShader*   m_HullShader {};
	ID3D11DomainShader* m_DomainShader {};
	
	ID3D11SamplerState* m_SampleStateWrap {};

	ID3D11InputLayout* m_Layout {};
	ID3D11Buffer* m_MatrixBuffer {};
	ID3D11Buffer* m_DepthMaterialBuffer {};
	ID3D11Buffer* m_TessellationBuffer {};
};