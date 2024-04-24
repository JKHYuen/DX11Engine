#include "Model.h"
#include <fstream>

bool Model::Initialize(ID3D11Device* device, const std::string& modelFilePath) {
	// Load in the model data.
	bool result = LoadModel(modelFilePath);
	if(!result) {
		return false;
	}

	CalculateModelVectors();

	// Initialize the vertex and index buffers.
	result = InitializeBuffers(device);
	if(!result) {
		return false;
	}

	return true;
}

void Model::Render(ID3D11DeviceContext* deviceContext, bool isPatchList) {
	// Put the vertex and index buffers on the graphics pipeline to prepare them for drawing.
	// Set vertex buffer stride and offset.
	unsigned int stride {sizeof(VertexType)};
	unsigned int offset {0};

	// Set the vertex buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Using patch list topology (Tessellation) is hard coded to triangles only
	if(isPatchList) {
		deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
	}
	else {
		deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}
}

bool Model::InitializeBuffers(ID3D11Device* device) {
	HRESULT result;

	// Create the vertex array.
	VertexType* vertices = new VertexType[m_VertexCount];

	// Create the index array.
	unsigned long* indices = new unsigned long[m_IndexCount];

	// Load the vertex array and index array with data.
	for(int i = 0; i < m_VertexCount; i++) {
		vertices[i].position = XMFLOAT3(m_Model[i].x, m_Model[i].y, m_Model[i].z);
		vertices[i].texture  = XMFLOAT2(m_Model[i].tu, m_Model[i].tv);
		vertices[i].normal   = XMFLOAT3(m_Model[i].nx, m_Model[i].ny, m_Model[i].nz);
		vertices[i].tangent  = XMFLOAT3(m_Model[i].tx, m_Model[i].ty, m_Model[i].tz);
		vertices[i].binormal = XMFLOAT3(m_Model[i].bx, m_Model[i].by, m_Model[i].bz);
		
		indices[i] = i;
	}

	// Set up the description of the static vertex buffer.
	D3D11_BUFFER_DESC vertexBufferDesc {};
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * m_VertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	D3D11_SUBRESOURCE_DATA vertexData {};
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Now create the vertex buffer.
	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_VertexBuffer);
	if(FAILED(result)) {
		return false;
	}

	// Set up the description of the static index buffer.
	D3D11_BUFFER_DESC indexBufferDesc {};
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * m_IndexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the index data.
	D3D11_SUBRESOURCE_DATA indexData {};
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer.
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_IndexBuffer);
	if(FAILED(result)) {
		return false;
	}

	// Release the arrays now that the vertex and index buffers have been created and loaded.
	delete[] vertices;
	vertices = nullptr;

	delete[] indices;
	indices = nullptr;

	return true;
}

bool Model::LoadModel(std::string filename) {
	std::ifstream fin {};

	// Open the model file.
	fin.open(filename);

	// If it could not open the file then exit.
	if(fin.fail()) {
		return false;
	}

	// Read up to the value of vertex count.
	char input {};
	while(input != ':') {
		fin.get(input);
	}

	// Read in the vertex count.
	fin >> m_VertexCount;

	// Set the number of indices to be the same as the vertex count.
	m_IndexCount = m_VertexCount;

	// Create the model using the vertex count that was read in.
	m_Model = new ModelType[m_VertexCount];

	// Read up to the beginning of the data.
	input = {};
	while(input != ':') {
		fin.get(input);
	}

	// Read in the vertex data.
	for(int i = 0; i < m_VertexCount; i++) {
		fin >> m_Model[i].x  >> m_Model[i].y  >> m_Model[i].z;
		fin >> m_Model[i].tu >> m_Model[i].tv;
		fin >> m_Model[i].nx >> m_Model[i].ny >> m_Model[i].nz;
		
		if(m_Model[i].x > m_Extents.x) {
			m_Extents.x = m_Model[i].x;
		}
		if(m_Model[i].y > m_Extents.y) {
			m_Extents.y = m_Model[i].y;
		}
		if(m_Model[i].z > m_Extents.z) {
			m_Extents.z = m_Model[i].z;
		}
	}

	// Close the model file.
	fin.close();

	return true;
}

void Model::CalculateModelVectors() {
	TempVertexType vertex1 {}, vertex2 {}, vertex3 {};
	VectorType tangent {}, binormal {};

	// Calculate the number of faces in the model.
	int faceCount = m_VertexCount / 3;

	// Initialize the index to the model data.
	int index = 0;

	// Go through all the faces and calculate the the tangent and binormal vectors.
	for(int i = 0; i < faceCount; i++) {
		// Get the three vertices for this face from the model.
		vertex1.x = m_Model[index].x;
		vertex1.y = m_Model[index].y;
		vertex1.z = m_Model[index].z;
		vertex1.tu = m_Model[index].tu;
		vertex1.tv = m_Model[index].tv;
		index++;

		vertex2.x = m_Model[index].x;
		vertex2.y = m_Model[index].y;
		vertex2.z = m_Model[index].z;
		vertex2.tu = m_Model[index].tu;
		vertex2.tv = m_Model[index].tv;
		index++;

		vertex3.x = m_Model[index].x;
		vertex3.y = m_Model[index].y;
		vertex3.z = m_Model[index].z;
		vertex3.tu = m_Model[index].tu;
		vertex3.tv = m_Model[index].tv;
		index++;

		CalculateTangentBinormal(vertex1, vertex2, vertex3, tangent, binormal);

		// Store the tangent and binormal for this face back in the model structure.
		m_Model[index - 1].tx = tangent.x;
		m_Model[index - 1].ty = tangent.y;
		m_Model[index - 1].tz = tangent.z;
		m_Model[index - 1].bx = binormal.x;
		m_Model[index - 1].by = binormal.y;
		m_Model[index - 1].bz = binormal.z;

		m_Model[index - 2].tx = tangent.x;
		m_Model[index - 2].ty = tangent.y;
		m_Model[index - 2].tz = tangent.z;
		m_Model[index - 2].bx = binormal.x;
		m_Model[index - 2].by = binormal.y;
		m_Model[index - 2].bz = binormal.z;

		m_Model[index - 3].tx = tangent.x;
		m_Model[index - 3].ty = tangent.y;
		m_Model[index - 3].tz = tangent.z;
		m_Model[index - 3].bx = binormal.x;
		m_Model[index - 3].by = binormal.y;
		m_Model[index - 3].bz = binormal.z;
	}
}

void Model::CalculateTangentBinormal(TempVertexType vertex1, TempVertexType vertex2, TempVertexType vertex3, VectorType& tangent, VectorType& binormal) {
	float vector1[3], vector2[3];
	float tuVector[2], tvVector[2];
	float den;
	float length;

	// Calculate the two vectors for this face.
	vector1[0] = vertex2.x - vertex1.x;
	vector1[1] = vertex2.y - vertex1.y;
	vector1[2] = vertex2.z - vertex1.z;

	vector2[0] = vertex3.x - vertex1.x;
	vector2[1] = vertex3.y - vertex1.y;
	vector2[2] = vertex3.z - vertex1.z;

	// Calculate the tu and tv texture space vectors.
	tuVector[0] = vertex2.tu - vertex1.tu;
	tvVector[0] = vertex2.tv - vertex1.tv;

	tuVector[1] = vertex3.tu - vertex1.tu;
	tvVector[1] = vertex3.tv - vertex1.tv;

	// Calculate the denominator of the tangent/binormal equation.
	den = 1.0f / (tuVector[0] * tvVector[1] - tuVector[1] * tvVector[0]);

	// Calculate the cross products and multiply by the coefficient to get the tangent and binormal.
	tangent.x = (tvVector[1] * vector1[0] - tvVector[0] * vector2[0]) * den;
	tangent.y = (tvVector[1] * vector1[1] - tvVector[0] * vector2[1]) * den;
	tangent.z = (tvVector[1] * vector1[2] - tvVector[0] * vector2[2]) * den;

	binormal.x = (tuVector[0] * vector2[0] - tuVector[1] * vector1[0]) * den;
	binormal.y = (tuVector[0] * vector2[1] - tuVector[1] * vector1[1]) * den;
	binormal.z = (tuVector[0] * vector2[2] - tuVector[1] * vector1[2]) * den;

	// Calculate the length of this normal.
	length = sqrt((tangent.x * tangent.x) + (tangent.y * tangent.y) + (tangent.z * tangent.z));

	// Normalize the normal and then store it
	tangent.x = tangent.x / length;
	tangent.y = tangent.y / length;
	tangent.z = tangent.z / length;

	// Calculate the length of this normal.
	length = sqrt((binormal.x * binormal.x) + (binormal.y * binormal.y) + (binormal.z * binormal.z));

	// Normalize the normal and then store it
	binormal.x = binormal.x / length;
	binormal.y = binormal.y / length;
	binormal.z = binormal.z / length;
}

void Model::Shutdown() {
	// Release the model texture.
	//for(auto tex : m_Textures) {
	//	tex.Shutdown();
	//}

	// Shutdown the vertex and index buffers.
	// Release the index buffer.
	if(m_IndexBuffer) {
		m_IndexBuffer->Release();
		m_IndexBuffer = nullptr;
	}

	// Release the vertex buffer.
	if(m_VertexBuffer) {
		m_VertexBuffer->Release();
		m_VertexBuffer = nullptr;
	}

	// Release the model data.
	if(m_Model) {
		delete[] m_Model;
		m_Model = nullptr;
	}
}

