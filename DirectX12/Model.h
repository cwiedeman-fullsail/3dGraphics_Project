#pragma once
#include <string>
#include <vector>
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")
#include "d3dx12.h" // official helper file provided by microsoft
using namespace std;

struct VEC3
{
	float x, y, z;
};

struct Vertex
{
	VEC3									pos;
	VEC3									uvw;
	VEC3									nrm;
};

struct Material_Attributes
{
	VEC3									Kd;
	float									d;
	VEC3									Ks;
	float									Ns;
	VEC3									Ka;
	float									sharpness;
	VEC3									Tf;
	float									Ni;
	VEC3									Ke;
	unsigned								illum;
};

struct Meshes
{
	string									name;
	unsigned								indexCount;
	unsigned								indexOffset;
	unsigned								materialIndex;
};

class Model
{
public:
	Model();
	~Model();
	string									modelName;
	vector<Vertex>							vertList;
	unsigned								vCount = 0;
	vector<float>							indexList;
	unsigned								iCount = 0;
	unsigned								meshCount = 0;
	unsigned								matCount = 0;
	vector<string>							matName;
	vector<Material_Attributes>				materials;
	vector<Meshes>							objects;
	D3D12_VERTEX_BUFFER_VIEW				vertexView;
	Microsoft::WRL::ComPtr<ID3D12Resource>	vertexBuffer;

	void addToVertList(Vertex V)
	{
		vertList.push_back(V);
	}
	void addToIndexList(float I)
	{
		indexList.push_back(I);
	}
	void buildMateralAttributeList(Material_Attributes mat)
	{
		materials.push_back(mat);
	}
	void buildMateralAttributeList(VEC3 _Kd, float _d, VEC3 _Ks, float _Ns, VEC3 _Ka, float _sharp, VEC3 _Tf, float _Ni, VEC3 _Ke, unsigned _ill)
	{
		Material_Attributes mat;
		mat.Kd = _Kd;
		mat.d = _d;
		mat.Ks = _Ks;
		mat.Ns = _Ns;
		mat.Ka = _Ka;
		mat.sharpness = _sharp;
		mat.Tf = _Tf;
		mat.Ni = _Ni;
		mat.Ke = _Ke;
		mat.illum = _ill;
		materials.push_back(mat);
	}
	void buildMeshList(Meshes mesh)
	{
		objects.push_back(mesh);
	}
	void buildMeshList(string _name, unsigned _indexCount, unsigned _indexOffset, unsigned _matIndex)
	{
		Meshes mesh;
		mesh.name = _name;
		mesh.indexCount = _indexCount;
		mesh.indexOffset = _indexOffset;
		mesh.materialIndex = _matIndex;
		objects.push_back(mesh);
	}

	void createVertexBuffer(ID3D12Device* _creator)
	{
		_creator->CreateCommittedResource( // using UPLOAD heap for simplicity
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // DEFAULT recommend  
			D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(Vertex) * vertList.size()),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer));
		// Transfer triangle data to the vertex buffer.
		UINT8* transferMemoryLocation;
		vertexBuffer->Map(0, &CD3DX12_RANGE(0, 0),
			reinterpret_cast<void**>(&transferMemoryLocation));
		memcpy(transferMemoryLocation, vertList.data(), sizeof(Vertex) * vertList.size());
		vertexBuffer->Unmap(0, nullptr);
		// Create a vertex View to send to a Draw() call.
		vertexView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vertexView.StrideInBytes = sizeof(Vertex);
		vertexView.SizeInBytes = sizeof(Vertex) * vertList.size();
	}
};

Model::Model()
{
	
}

Model::~Model()
{
}