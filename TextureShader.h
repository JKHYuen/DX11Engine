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
	bool Render(ID3D11DeviceContext*, int, XMMATRIX, XMMATRIX, XMMATRIX, ID3D11ShaderResourceView*);

	ID3D11VertexShader* GetVertexShader() const { return m_VertexShader; }

private:
	struct MatrixBufferType {
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
	};

private:
	bool InitializeShader(ID3D11Device*, HWND, WCHAR*, WCHAR*);
	void OutputShaderErrorMessage(ID3D10Blob*, HWND, WCHAR*);

private:
	ID3D11VertexShader* m_VertexShader {};
	ID3D11PixelShader*  m_PixelShader {};
	ID3D11InputLayout*  m_Layout {};
	ID3D11Buffer*       m_MatrixBuffer {};
	ID3D11SamplerState* m_SampleState {};
};