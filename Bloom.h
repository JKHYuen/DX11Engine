#pragma once
#include <array>

#include <d3d11.h>
#include <directxmath.h>
using namespace DirectX;

class RenderTexture;
class TextureShader;

// Implementation based on: https://catlikecoding.com/unity/tutorials/advanced-rendering/bloom/
class Bloom {
public:
	Bloom() {}
	Bloom(const Bloom&) {}
	~Bloom() {}

	bool Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND hwnd, RenderTexture* screenTexture, ID3D11VertexShader* screenRenderVertexShader);
	bool InitializeShader(ID3D11Device* device, HWND hwnd, std::wstring shaderName);
	bool Render(ID3D11DeviceContext* deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ID3D11ShaderResourceView* textureSRV);
	void Shutdown();

private:
	struct MatrixBufferType {
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
	};

private:
	void OutputShaderErrorMessage(ID3D10Blob* errorMessage, HWND hwnd, WCHAR* shaderFilename);

private:
	ID3D11VertexShader* m_PostProcVertexShaderInstance {};
	std::array<RenderTexture*, 16> m_RenderTexArray {};

	ID3D11VertexShader* m_VertexShader {};
	ID3D11PixelShader*  m_PixelShader {};
	ID3D11InputLayout*  m_Layout {};
	ID3D11Buffer*       m_MatrixBuffer {};
	ID3D11SamplerState* m_SampleState {};
};
