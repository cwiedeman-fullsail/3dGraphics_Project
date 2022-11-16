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
	VEC3()
	{};
	VEC3(float _x, float _y, float _z)
	{
		x = _x;
		y = _y;
		z = _z;
	}
};
struct VEC4
{
	float x, y, z, w;
	VEC4()
	{};
	VEC4(float _x, float _y, float _z,float _w)
	{
		x = _x;
		y = _y;
		z = _z;
		w = _w;
	}
};
struct VEC4SET
{
	GW::MATH::GVECTORF x;
	GW::MATH::GVECTORF y;
	GW::MATH::GVECTORF z;
	GW::MATH::GVECTORF w;
	VEC4SET(GW::MATH::GVECTORF _x, GW::MATH::GVECTORF _y, GW::MATH::GVECTORF _z, GW::MATH::GVECTORF _w)
	{
		x = _x;
		y = _y;
		z = _z;
		w = _w;
	}
};

struct Vertex
{
	VEC4									pos;
	VEC4									uvw;
	VEC3									nrm;
	Vertex() {};
	Vertex(VEC4 _pos, VEC4 _uvw, VEC3 _nrm)
	{
		pos = _pos;
		uvw = _uvw;
		nrm = _nrm;
	}
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

	vector<Material_Attributes>						materials;
	vector<Meshes>									objects;
	vector<Vertex>									boundVList;
	vector<unsigned>								boundIList;
	vector<MESH_DATA>								meshAndMaterialDataList;
	MESH_DATA										boundingMesh;
	SCENE_DATA										CamandLight;
	SCENE_DATA										MiniMap;
	GW::MATH::GMATRIXF								worldMatrix;
	
	std::vector<GW::MATH::GVECTORF>					AABB_List;


	D3D12_VERTEX_BUFFER_VIEW						vertexView;
	Microsoft::WRL::ComPtr<ID3D12Resource>			vertexBuffer;
	D3D12_INDEX_BUFFER_VIEW							indexView;
	Microsoft::WRL::ComPtr<ID3D12Resource>			indexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource>			constantBuffer;
	D3D12_VERTEX_BUFFER_VIEW						boundingvertexView;
	Microsoft::WRL::ComPtr<ID3D12Resource>			boundingvertexBuffer;
	D3D12_INDEX_BUFFER_VIEW							boundingindexView;
	Microsoft::WRL::ComPtr<ID3D12Resource>			boundingindexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource>			BoundingconstantBuffer;
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
	void buildMeshList(unsigned _indexCount, unsigned _indexOffset)
	{
		Meshes mesh;
		mesh.indexCount = _indexCount;
		mesh.indexOffset = _indexOffset;
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
		cbSize = ((sizeof(SCENE_DATA) * 2) + (sizeof(MESH_DATA) * matCount)) * _frames;
		chunkSize = (sizeof(SCENE_DATA) * 2) + (sizeof(MESH_DATA) * matCount);
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
		memcpy(transferMemoryLocation + sizeof(SCENE_DATA), &MiniMap, sizeof(SCENE_DATA));
		memcpy(transferMemoryLocation + chunkSize + sizeof(SCENE_DATA), &MiniMap, sizeof(SCENE_DATA));
		memcpy(transferMemoryLocation + (sizeof(SCENE_DATA) * 2) + (sizeof(MESH_DATA) * _matIndex), &_mesh, sizeof(MESH_DATA));
		memcpy(transferMemoryLocation + chunkSize + (sizeof(SCENE_DATA) * 2) + (sizeof(MESH_DATA) * _matIndex), &_mesh, sizeof(MESH_DATA));
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
	void createBoundingVertexBuffer(ID3D12Device* _creator)
	{
		_creator->CreateCommittedResource( // using UPLOAD heap for simplicity
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // DEFAULT recommend  
			D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(Vertex) * boundVList.size()),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&boundingvertexBuffer));
		// Transfer triangle data to the vertex buffer.
		UINT8* transferMemoryLocation;
		boundingvertexBuffer->Map(0, &CD3DX12_RANGE(0, 0),
			reinterpret_cast<void**>(&transferMemoryLocation));
		memcpy(transferMemoryLocation, boundVList.data(), sizeof(Vertex) * boundVList.size());
		boundingvertexBuffer->Unmap(0, nullptr);
		// Create a vertex View to send to a Draw() call.
		boundingvertexView.BufferLocation = boundingvertexBuffer->GetGPUVirtualAddress();
		boundingvertexView.StrideInBytes = sizeof(Vertex);
		boundingvertexView.SizeInBytes = sizeof(Vertex) * boundVList.size();
	}

	void createBoundingIndexBuffer(ID3D12Device* _creator)
	{
		_creator->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(float) * boundIList.size()),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&boundingindexBuffer));
		UINT8* transferMemoryLocation;
		boundingindexBuffer->Map(0, &CD3DX12_RANGE(0, 0),
			reinterpret_cast<void**>(&transferMemoryLocation));
		memcpy(transferMemoryLocation, boundIList.data(), sizeof(float) * boundIList.size());
		boundingindexBuffer->Unmap(0, nullptr);
		boundingindexView.BufferLocation = boundingindexBuffer->GetGPUVirtualAddress();
		boundingindexView.Format = DXGI_FORMAT_R32_UINT;
		boundingindexView.SizeInBytes = sizeof(float) * boundIList.size();
	}
	void createBoundingConstantBuffer(ID3D12Device* _creator, int _frames)
	{
		cbSize = ((sizeof(SCENE_DATA) * 2) + (sizeof(MESH_DATA) * matCount)) * _frames;
		chunkSize = (sizeof(SCENE_DATA) * 2) + (sizeof(MESH_DATA) * matCount);
		_creator->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(cbSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(BoundingconstantBuffer.GetAddressOf()));
	}

	void createBoundingCBView()
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = BoundingconstantBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = cbSize;
	}

	void loadBoundingMaterialsToGPU(MESH_DATA* _mesh)
	{
		UINT8* transferMemoryLocation;
		BoundingconstantBuffer->Map(0, &CD3DX12_RANGE(0, 0),
			reinterpret_cast<void**>(&transferMemoryLocation));
		memcpy(transferMemoryLocation, &CamandLight, sizeof(SCENE_DATA));
		memcpy(transferMemoryLocation + chunkSize, &CamandLight, sizeof(SCENE_DATA));
		memcpy(transferMemoryLocation + sizeof(SCENE_DATA), &MiniMap, sizeof(SCENE_DATA));
		memcpy(transferMemoryLocation + chunkSize + sizeof(SCENE_DATA), &MiniMap, sizeof(SCENE_DATA));
		memcpy(transferMemoryLocation + (sizeof(SCENE_DATA) * 2), &_mesh, sizeof(MESH_DATA));
		memcpy(transferMemoryLocation + chunkSize + (sizeof(SCENE_DATA) * 2), &_mesh, sizeof(MESH_DATA));
		BoundingconstantBuffer->Unmap(0, nullptr);

	}
	void GenerateBounds(vector<Vertex> _vList)
	{
		float minX = NULL;
		float maxX = NULL;
		float minY = NULL;
		float maxY = NULL;
		float minZ = NULL;
		float maxZ = NULL;

		for (size_t i = 0; i < _vList.size(); i++)
		{
			if (minX == NULL)
			{
				minX = _vList[i].pos.x;
			}
			else
			{
				if (_vList[i].pos.x < minX)
				{
					minX = _vList[i].pos.x;
				}
			}
			if (maxX == NULL)
			{
				maxX = _vList[i].pos.x;
			}
			else
			{
				if (_vList[i].pos.x > maxX)
				{
					maxX = _vList[i].pos.x;
				}
			}
			if (minY == NULL)
			{
				minY = _vList[i].pos.y;
			}
			else
			{
				if (_vList[i].pos.y < minY)
				{
					minY = _vList[i].pos.y;
				}
			}
			if (maxY == NULL)
			{
				maxY = _vList[i].pos.y;
			}
			else
			{
				if (_vList[i].pos.y > maxY)
				{
					maxY = _vList[i].pos.y;
				}
			}
			if (minZ == NULL)
			{
				minZ = _vList[i].pos.z;
			}
			else
			{
				if (_vList[i].pos.z < minZ)
				{
					minZ = _vList[i].pos.z;
				}
			}
			if (maxZ == NULL)
			{
				maxZ = _vList[i].pos.z;
			}
			else
			{
				if (_vList[i].pos.z > maxZ)
				{
					maxZ = _vList[i].pos.z;
				}
			}
		}
		Vertex _1 = { VEC4(minX, maxY, minZ, 1.0f),VEC4(0,0,0,0), VEC3(0,0,0)};
		Vertex _2 = { VEC4(minX, maxY, maxZ, 1.0f),VEC4(0,0,0,0), VEC3(0,0,0)};
		Vertex _3 = { VEC4(maxX, maxY, maxZ, 1.0f),VEC4(0,0,0,0), VEC3(0,0,0)};
		Vertex _4 = { VEC4(maxX, maxY, minZ, 1.0f),VEC4(0,0,0,0), VEC3(0,0,0)};
		Vertex _5 = { VEC4(minX, minY, minZ, 1.0f),VEC4(0,0,0,0), VEC3(0,0,0)};
		Vertex _6 = { VEC4(minX, minY, maxZ, 1.0f),VEC4(0,0,0,0), VEC3(0,0,0)};
		Vertex _7 = { VEC4(maxX, minY, maxZ, 1.0f),VEC4(0,0,0,0), VEC3(0,0,0)};
		Vertex _8 = { VEC4(maxX, minY, minZ, 1.0f),VEC4(0,0,0,0), VEC3(0,0,0)};
		boundVList.push_back(_1);
		boundVList.push_back(_2);
		boundVList.push_back(_3);
		boundVList.push_back(_4);
		boundVList.push_back(_5);
		boundVList.push_back(_6);
		boundVList.push_back(_7);
		boundVList.push_back(_8);
		boundIList = {
		0,1,
		1,2,
		2,3,
		3,0,
		4,5,
		5,6,
		6,7,
		7,4,
		0,4,
		3,7,
		1,5,
		2,6
		};

	}

	std::vector<GW::MATH::GVECTORF> CreateAABB(std::vector<Vertex>& vertPosArray)
	{
		GW::MATH::GVECTORF minVertex = { FLT_MAX, FLT_MAX, FLT_MAX };
		GW::MATH::GVECTORF maxVertex = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

		for (UINT i = 0; i < vertPosArray.size(); i++)
		{
			minVertex.x = min(minVertex.x, vertPosArray[i].pos.x);
			minVertex.y = min(minVertex.y, vertPosArray[i].pos.y);
			minVertex.z = min(minVertex.z, vertPosArray[i].pos.z);

			//Get the largest vertex 
			maxVertex.x = max(maxVertex.x, vertPosArray[i].pos.x);
			maxVertex.y = max(maxVertex.y, vertPosArray[i].pos.y);
			maxVertex.z = max(maxVertex.z, vertPosArray[i].pos.z);
		}
		std::vector<GW::MATH::GVECTORF> AABB;

		AABB.push_back(minVertex);
		AABB.push_back(maxVertex);

		return AABB;
	}
};

Model::Model()
{

}

Model::~Model()
{

}