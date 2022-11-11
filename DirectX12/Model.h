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
struct VEC4
{
	float x, y, z, w;
};

struct Vertex
{
	VEC4									pos;
	VEC4									uvw;
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
	unsigned								indexCount = 0;
	unsigned								indexOffset = 0;
	unsigned								materialIndex = 0;
};

struct SCENE_DATA
{
	GW::MATH::GVECTORF sunDirection, sunColor;
	GW::MATH::GMATRIXF viewMatrix, projectionMatrix;
	GW::MATH::GVECTORF sumAmb, camPOS;
	GW::MATH::GVECTORF padding[4];
};

struct MESH_DATA
{
	GW::MATH::GMATRIXF worldMatrix;
	Material_Attributes material;
	unsigned padding[28];
};

class Model
{
private:
	UINT64 cbSize;
	UINT chunkSize;
public:
	Model();
	~Model();
	string											modelName;
	string											modelType;
	vector<Vertex>									vertList;
	unsigned										vCount = 0;
	vector<unsigned>								indexList;
	unsigned										iCount = 0;
	unsigned										meshCount = 0;
	unsigned										matCount = 0;
	GW::MATH::GMATRIXF								positionMatrix;

	vector<Material_Attributes>						materials;
	vector<Meshes>									objects;
	vector<MESH_DATA>								MeshDataList;
	SCENE_DATA										CamandLight;

	D3D12_VERTEX_BUFFER_VIEW						vertexView;
	Microsoft::WRL::ComPtr<ID3D12Resource>			vertexBuffer;
	D3D12_INDEX_BUFFER_VIEW							indexView;
	Microsoft::WRL::ComPtr<ID3D12Resource>			indexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource>			constantBuffer;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>	dHeap;
	CD3DX12_CPU_DESCRIPTOR_HANDLE					cpuHandle;

	void addToVertList(Vertex V)
	{
		V.pos.w = 1;
		V.uvw.w = 1;
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
	void buildMeshList(unsigned _indexCount, unsigned _indexOffset, unsigned _matIndex)
	{
		Meshes mesh;
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

	void createIndexBuffer(ID3D12Device* _creator)
	{
		_creator->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(float) * indexList.size()),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexBuffer));
		UINT8* transferMemoryLocation;
		indexBuffer->Map(0, &CD3DX12_RANGE(0, 0),
			reinterpret_cast<void**>(&transferMemoryLocation));
		memcpy(transferMemoryLocation, indexList.data(), sizeof(float) * indexList.size());
		indexBuffer->Unmap(0, nullptr);
		indexView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
		indexView.Format = DXGI_FORMAT_R32_UINT;
		indexView.SizeInBytes = sizeof(float) * indexList.size();
	}

	void createConstantBuffer(ID3D12Device* _creator, int _frames)
	{
		cbSize = (sizeof(SCENE_DATA) + (sizeof(MESH_DATA) * matCount)) * _frames;
		chunkSize = sizeof(SCENE_DATA) + (sizeof(MESH_DATA) * matCount);
		_creator->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(cbSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(constantBuffer.GetAddressOf()));
	}
	void loadMaterialsToGPU(MESH_DATA* _mesh, int _matIndex)
	{
		UINT8* transferMemoryLocation;
		constantBuffer->Map(0, &CD3DX12_RANGE(0, 0),
			reinterpret_cast<void**>(&transferMemoryLocation));
		memcpy(transferMemoryLocation, &CamandLight, sizeof(SCENE_DATA));
		memcpy(transferMemoryLocation + chunkSize, &CamandLight, sizeof(SCENE_DATA));
		memcpy(transferMemoryLocation + sizeof(SCENE_DATA) + (sizeof(MESH_DATA) * _matIndex), &_mesh, sizeof(MESH_DATA));
		memcpy(transferMemoryLocation + chunkSize + sizeof(SCENE_DATA) + (sizeof(MESH_DATA) * _matIndex), &_mesh, sizeof(MESH_DATA));
		constantBuffer->Unmap(0, nullptr);

	}

	void createDescriptorHeap(ID3D12Device* _creator)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 1;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		_creator->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&dHeap));
	}

	void createCBView()
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = constantBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = cbSize;
	}

	void createCPUHandle()
	{
		cpuHandle = dHeap->GetCPUDescriptorHandleForHeapStart();
	}
};

Model::Model()
{

}

Model::~Model()
{
}