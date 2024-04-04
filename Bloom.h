#pragma once
#include <vector>
#include <string>

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

	bool Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND hwnd, RenderTexture* screenTexture, TextureShader* screenRenderShader);
	bool InitializeShader(ID3D11Device* device, HWND hwnd, std::wstring shaderName);
	bool Render(ID3D11DeviceContext* deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ID3D11ShaderResourceView* textureSRV);
	void Shutdown();

private:
	struct MatrixBufferType {
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
	};

	struct BloomParamBufferType {
		XMFLOAT4 filter;
		float boxSampleDelta;
		float intensity;
		// use prefilter if not 0.0f
		float usePrefilter;
		float padding;
	};

private:
	void OutputShaderErrorMessage(ID3D10Blob* errorMessage, HWND hwnd, WCHAR* shaderFilename);

private:
	// Bloom params
	float m_Threshold {};
	float m_SoftThreshold {};
	float m_BoxSampleDelta {};
	float m_Intensity {};
	bool  m_UsePrefilter {};

	ID3D11VertexShader* m_PostProcVertexShaderInstance {};
	std::vector<RenderTexture*> m_RenderTexures {};

	int m_CurrentMaxIteration {};

	ID3D11VertexShader* m_ScreenVertexShaderInstance {};
	ID3D11PixelShader*  m_PixelShader {};
	ID3D11InputLayout*  m_screenShaderLayoutInstance {};
	ID3D11Buffer*       m_MatrixBuffer {};
	ID3D11Buffer*       m_BloomParamBuffer {};
	ID3D11SamplerState* m_SampleState {};
};
