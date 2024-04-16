#pragma once
#include <vector>
#include <string>

#include <d3d11.h>
#include <directxmath.h>
using namespace DirectX;

class RenderTexture;
class TextureShader;
class D3DInstance;

// Implementation based on: https://catlikecoding.com/unity/tutorials/advanced-rendering/bloom/
// Issue: very bright shimmering caused by extremely high specular values in a small pixel fragment area. Particularly with near-zero roughness surfaces.
// Possible solutions:
//  - Use bloom after tonemapping
//		- Does not look as good, colors smudged together like in LDR rendering
//  - Try to avoid near zero roughness and/or sharp roughness contrast by editing texture or PBR shader
//  - Limit roughness in PBR shader
//		- Drawback: can't have perfectly reflective materials
//	- adjust bloom parameters (use very high threshold)
//		- Drawback: less noticable bloom
//  - Aliasing exacerbates issue, use AA
//		- This engine currently as no AA (will most likely need TAA), this would only lessen the flickering, not fix it entirely
//		- Unreal 4 (in which my PBR shader is based on) has the same flickering issues, engine uses TAA to alleviate issue

class Bloom {
public:
	Bloom() {}
	Bloom(const Bloom&) {}
	~Bloom() {}

	bool Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND hwnd, RenderTexture* screenTexture, TextureShader* screenRenderShader);
	bool RenderEffect(D3DInstance* d3dInstance, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ID3D11ShaderResourceView* textureSRV);
	bool Render(ID3D11DeviceContext* deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ID3D11ShaderResourceView* textureSRV, bool b_IsFinalPass) const;
	void Shutdown();

	// Resizes (recreates) all render textures if they have been generated before
	bool GenerateRenderTextures(ID3D11Device* device, ID3D11DeviceContext* deviceContext, int screenWidth, int screenHeight, float nearZ, float farZ);

	RenderTexture* GetDebugBloomTexture() const { return m_RenderTexures[0]; }
	RenderTexture* GetBloomOutput() const { return m_BloomOutputTexture; }

	float GetIntensity()     const { return m_Intensity; }
	float GetThreshold()     const { return m_Threshold; }
	float GetSoftThreshold() const { return m_SoftThreshold; }

	void SetThreshold(float threshold) { m_Threshold = threshold; }
	void SetSoftThreshold(float softThreshold) { m_SoftThreshold = softThreshold; }
	void SetIntensity(float intensity) { m_Intensity = intensity; }

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
		float b_UsePrefilter;
		float b_UseFinalPass;
	};

private:
	// Bloom params
	float m_Intensity = 1.0f;
	float m_Threshold = 40.0f;
	float m_SoftThreshold = 0.9f;

	float m_BoxSampleDelta {};
	bool  mb_UsePrefilter {};

	std::vector<RenderTexture*> m_RenderTexures {};
	RenderTexture* m_BloomOutputTexture {};

	int m_IterationCount {};

	ID3D11VertexShader* m_ScreenVertexShaderInstance {};
	ID3D11InputLayout*  m_screenShaderLayoutInstance {};

	ID3D11PixelShader*  m_PixelShader {};
	ID3D11Buffer*       m_MatrixBuffer {};
	ID3D11Buffer*       m_BloomParamBuffer {};
	ID3D11SamplerState* m_SampleState {};
};
