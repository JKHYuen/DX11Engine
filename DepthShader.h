#pragma once

#include <d3d11.h>
#include <directxmath.h>
using namespace DirectX;

class DepthShader {
private:
	struct MatrixBufferType {
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
	};

public:
	DepthShader();
	DepthShader(const DepthShader&);
	~DepthShader();

	bool Initialize(ID3D11Device*, HWND);
	void Shutdown();
	bool Render(ID3D11DeviceContext* deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix,
		XMMATRIX projectionMatrix);

private:
	ID3D11VertexShader* m_VertexShader {};
	ID3D11PixelShader* m_PixelShader {};
	ID3D11InputLayout* m_Layout {};
	ID3D11Buffer* m_MatrixBuffer {};
};