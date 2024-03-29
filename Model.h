#pragma once
#include <d3d11.h>
#include <directxmath.h>
#include <vector>

#include "Texture.h"
using namespace DirectX;

class Model {
private:
	struct VertexType {
		XMFLOAT3 position;
		XMFLOAT2 texture;
		XMFLOAT3 normal;
		XMFLOAT3 tangent;
		XMFLOAT3 binormal;
	};

	struct ModelType {
		float x, y, z;
		float tu, tv;
		float nx, ny, nz;
		float tx, ty, tz;
		float bx, by, bz;
	};

	struct TempVertexType {
		float x, y, z;
		float tu, tv;
		float nx, ny, nz;
	};

	struct VectorType {
		float x, y, z;
	};

	void CalculateModelVectors();
	void CalculateTangentBinormal(TempVertexType, TempVertexType, TempVertexType, VectorType&, VectorType&);

public:
	Model() {}
	Model(const Model&) {}
	~Model() {}

	bool Initialize(ID3D11Device*, ID3D11DeviceContext*, const std::string& modelFilePath);
	void Shutdown();
	void Render(ID3D11DeviceContext*);

	int GetIndexCount();

private:
	bool InitializeBuffers(ID3D11Device*);
	bool LoadModel(std::string);

private:
	ID3D11Buffer* m_VertexBuffer {};
	ID3D11Buffer* m_IndexBuffer  {};
	int m_VertexCount {};
	int m_IndexCount  {};

	ModelType* m_Model {};
};
