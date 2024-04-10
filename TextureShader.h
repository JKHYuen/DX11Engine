#pragma once

#include <d3d11.h>
#include <directxmath.h>
using namespace DirectX;

class TextureShader {
public:
	TextureShader() {}
	TextureShader(const TextureShader&) {}
	~TextureShader() {}

	bool Initialize(ID3D11Device*, HWND, bool isPostProcessShader);
	void Shutdown();
	bool Render(ID3D11DeviceContext* deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ID3D11ShaderResourceView* textureSRV) const;

	ID3D11VertexShader* GetVertexShader() const { return m_VertexShader; }
	ID3D11InputLayout* GetShaderInputLayout() const { return m_Layout; }

private:
	struct MatrixBufferType {
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
	};

private:
	bool InitializeShader(ID3D11Device*, HWND, WCHAR*, WCHAR*);

private:
	ID3D11VertexShader* m_VertexShader {};
	ID3D11PixelShader*  m_PixelShader {};
	ID3D11InputLayout*  m_Layout {};
	ID3D11Buffer*       m_MatrixBuffer {};
	ID3D11SamplerState* m_SampleState {};
};