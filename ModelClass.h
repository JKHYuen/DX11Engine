#pragma once
#include <d3d11.h>
#include <directxmath.h>
#include <fstream>
#include <vector>

#include "textureclass.h"
using namespace DirectX;

class ModelClass {
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
	ModelClass();
	ModelClass(const ModelClass&);
	~ModelClass();

	bool Initialize(ID3D11Device*, ID3D11DeviceContext*, const std::string& modelFilename, const std::vector<std::string>& textureFileNames, bool isCubeMap = false);
	void Shutdown();
	void Render(ID3D11DeviceContext*);

	int GetIndexCount();
	ID3D11ShaderResourceView* GetTexture(int);

private:
	bool InitializeBuffers(ID3D11Device*);
	bool LoadModel(std::string);

private:
	ID3D11Buffer* m_vertexBuffer {};
	ID3D11Buffer* m_indexBuffer  {};
	int m_vertexCount {};
	int m_indexCount  {};

	std::vector<TextureClass> m_textures {};

	ModelType* m_model {};
};
