// minimalistic code to draw a single triangle, this is not part of the API.
// required for compiling shaders on the fly, consider pre-compiling instead
#include "Model.h";
#include "FileIO.h"



std::string ShaderAsString(const char* shaderFilePath) {
	std::string output;
	unsigned int stringLength = 0;
	GW::SYSTEM::GFile file; file.Create();
	file.GetFileSize(shaderFilePath, stringLength);
	if (stringLength && +file.OpenBinaryRead(shaderFilePath)) {
		output.resize(stringLength);
		file.Read(&output[0], stringLength);
	}
	else
		std::cout << "ERROR: Shader Source File \"" << shaderFilePath << "\" Not Found!" << std::endl;
	return output;
}

vector<Model> UniqueObjectList;
GW::MATH::GMATRIXF worldM = GW::MATH::GIdentityMatrixF;
FileIO files;

// Creation, Rendering & Cleanup
class Renderer
{
	string gameLevelPath = "../Test/GameLevelTest2.txt";

	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GDirectX12Surface d3d;
	GW::MATH::GMatrix Math;

	// what we need at a minimum to draw a triangle
	Microsoft::WRL::ComPtr<ID3D12RootSignature>	rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState>	pipeline;

	//Matrixes
	GW::MATH::GMATRIXF viewM = GW::MATH::GIdentityMatrixF;
	GW::MATH::GMATRIXF projM = GW::MATH::GIdentityMatrixF;
	//directional light
	GW::MATH::GVECTORF light_direction = { -1.0f, -1.0f, 2.0f };
	GW::MATH::GVECTORF light_color = { 0.9f, 0.9f, 1.0f, 1.0f };

public:
	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GDirectX12Surface _d3d)
	{
		Math.Create();
		win = _win;
		d3d = _d3d;
		ID3D12Device* creator;
		d3d.GetDevice((void**)&creator);

		//Create Matrixes
		//view
		GW::MATH::GVECTORF eye = { 5.75f, 5.25f, -10.5f };
		GW::MATH::GVECTORF at = { 0 };
		GW::MATH::GVECTORF up = { 0.0f, 1.0f, 0.0f };
		Math.LookAtLHF(eye, at, up, viewM);

		//projection
		UINT width;
		UINT height;
		float aspect;
		win.GetWidth(width);
		win.GetHeight(height);
		aspect = (float)width / (float)height;
		Math.ProjectionDirectXLHF(G_DEGREE_TO_RADIAN_F(65), aspect, 0.1f, 100, projM);

		SCENE_DATA camerAndLights;
		GW::MATH::GVector::NormalizeF(light_direction, camerAndLights.sunDirection);
		camerAndLights.sunColor = light_color;
		camerAndLights.viewMatrix = viewM;
		camerAndLights.projectionMatrix = projM;
		camerAndLights.sumAmb = { 0.25f, 0.25f, 0.35f, 1 };

		//caculate Camera Position
		GW::MATH::GMATRIXF temp;
		Math.InverseF(viewM, temp);
		GW::MATH::GVECTORF temp2;
		Math.GetTranslationF(temp, temp2);
		camerAndLights.camPOS = temp2;

		DXGI_SWAP_CHAIN_DESC sw_desc;
		Microsoft::WRL::ComPtr <IDXGISwapChain4> test = nullptr;
		d3d.GetSwapchain4((void**)&test);
		test->GetDesc(&sw_desc);
		int frames = sw_desc.BufferCount;



		//set h2b files read folder
		files.h2BLocationFolder = "Test";

		//read files
		files.ReadFile(gameLevelPath);
		files.ReadH2B();

		for (size_t i = 0; i < files.meshData.size(); i++)
		{
			Model M;
			M.modelName = files.nameList[i];
			M.vCount = files.meshData[i].vertexCount;
			M.iCount = files.meshData[i].indexCount;
			M.matCount = files.meshData[i].materialCount;
			M.meshCount = files.meshData[i].meshCount;
			M.createConstantBuffer(creator, frames);
			for (size_t j = 0; j < files.meshData[i].materialCount; j++)
			{
				M.buildMateralAttributeList(
					VEC3{ files.meshData[i].materials[j].attrib.Kd.x, files.meshData[i].materials[j].attrib.Kd.y,files.meshData[i].materials[j].attrib.Kd.z },
					files.meshData[i].materials[j].attrib.d,
					VEC3{ files.meshData[i].materials[j].attrib.Ks.x,files.meshData[i].materials[j].attrib.Ks.y,files.meshData[i].materials[j].attrib.Ks.z },
					files.meshData[i].materials[j].attrib.Ns,
					VEC3{ files.meshData[i].materials[j].attrib.Ka.x,files.meshData[i].materials[j].attrib.Ka.y,files.meshData[i].materials[j].attrib.Ka.z },
					files.meshData[i].materials[j].attrib.sharpness,
					VEC3{ files.meshData[i].materials[j].attrib.Tf.x, files.meshData[i].materials[j].attrib.Tf.y, files.meshData[i].materials[j].attrib.Tf.z },
					files.meshData[i].materials[j].attrib.Ni,
					VEC3{ files.meshData[i].materials[j].attrib.Ke.x, files.meshData[i].materials[j].attrib.Ke.y,files.meshData[i].materials[j].attrib.Ke.z },
					files.meshData[i].materials[j].attrib.illum);
				M.buildMeshList(files.meshData[i].meshes[j].drawInfo.indexCount, files.meshData[i].meshes[j].drawInfo.indexOffset, files.meshData[i].meshes[j].materialIndex);
				MESH_DATA mesh;
				mesh.worldMatrix = worldM;
				mesh.material = M.materials[j];
				M.objects[i].materialIndex = j;
				M.MeshDataList.push_back(mesh);
				M.loadMaterialsToGPU(camerAndLights, &M.MeshDataList[j], j);
				M.createDescriptorHeap(creator);
				M.createCBView();
			}

			// Create Vertex Buffer
			for (size_t j = 0; j < files.meshData[i].vertices.size(); j++)
			{
				Vertex v;
				v.pos.x = files.meshData[i].vertices[j].pos.x;
				v.pos.y = files.meshData[i].vertices[j].pos.y;
				v.pos.z = files.meshData[i].vertices[j].pos.z;
				v.uvw.x = files.meshData[i].vertices[j].uvw.x;
				v.uvw.y = files.meshData[i].vertices[j].uvw.y;
				v.uvw.z = files.meshData[i].vertices[j].uvw.z;
				v.nrm.x = files.meshData[i].vertices[j].nrm.x;
				v.nrm.y = files.meshData[i].vertices[j].nrm.y;
				v.nrm.z = files.meshData[i].vertices[j].nrm.z;
				M.addToVertList(v);
			}
			M.createVertexBuffer(creator);
			for (size_t j = 0; j < files.meshData[i].indices.size(); j++)
			{
				M.addToIndexList(files.meshData[i].indices[j]);
			}
			M.createIndexBuffer(creator);

			UniqueObjectList.push_back(M);
		}

		// Create Vertex Shader
		UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
		compilerFlags |= D3DCOMPILE_DEBUG;
#endif
		Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, errors;
		std::string vShade = ShaderAsString("../vShader.hlsl");
		if (FAILED(D3DCompile(vShade.c_str(), vShade.length(),
			nullptr, nullptr, nullptr, "main", "vs_5_1", compilerFlags, 0,
			vsBlob.GetAddressOf(), errors.GetAddressOf())))
		{
			std::cout << (char*)errors->GetBufferPointer() << std::endl;
			abort();
		}

		// Create Pixel Shader
		Microsoft::WRL::ComPtr<ID3DBlob> psBlob; errors.Reset();
		std::string pShade = ShaderAsString("../pShader.hlsl");
		if (FAILED(D3DCompile(pShade.c_str(), pShade.length(),
			nullptr, nullptr, nullptr, "main", "ps_5_1", compilerFlags, 0,
			psBlob.GetAddressOf(), errors.GetAddressOf())))
		{
			std::cout << (char*)errors->GetBufferPointer() << std::endl;
			abort();
		}

		// Create Input Layout
		D3D12_INPUT_ELEMENT_DESC format[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		};

		CD3DX12_ROOT_PARAMETER root1;
		CD3DX12_ROOT_PARAMETER root2;

		root1.InitAsConstantBufferView(0, 0);
		root2.InitAsConstantBufferView(1, 0);

		CD3DX12_ROOT_PARAMETER rArr[2] = { root1,root2 };

		// create root signature
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(2, rArr, 0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		Microsoft::WRL::ComPtr<ID3DBlob> signature;
		D3D12SerializeRootSignature(&rootSignatureDesc,
			D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errors);
		creator->CreateRootSignature(0, signature->GetBufferPointer(),
			signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));



		// create pipeline state
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psDesc;
		ZeroMemory(&psDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		psDesc.InputLayout = { format, ARRAYSIZE(format) };
		psDesc.pRootSignature = rootSignature.Get();
		psDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
		psDesc.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());
		psDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psDesc.SampleMask = UINT_MAX;
		psDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psDesc.NumRenderTargets = 1;
		psDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psDesc.SampleDesc.Count = 1;
		creator->CreateGraphicsPipelineState(&psDesc, IID_PPV_ARGS(&pipeline));
		// free temporary handle
		creator->Release();
	}
	void Render()
	{
		// grab the context & render target
		ID3D12GraphicsCommandList* cmd;
		D3D12_CPU_DESCRIPTOR_HANDLE rtv;
		D3D12_CPU_DESCRIPTOR_HANDLE dsv;
		d3d.GetCommandList((void**)&cmd);
		d3d.GetCurrentRenderTargetView((void**)&rtv);
		d3d.GetDepthStencilView((void**)&dsv);
		// setup the pipeline
		cmd->SetGraphicsRootSignature(rootSignature.Get());
		cmd->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
		cmd->SetPipelineState(pipeline.Get());
		// now we can draw
		for (size_t i = 0; i < files.gameLevelObjects.size(); i++)
		{
			cmd->SetDescriptorHeaps(1, UniqueObjectList[files.gameLevelObjects[i].baseModelIndex].dHeap.GetAddressOf());
			cmd->SetGraphicsRootConstantBufferView(0, UniqueObjectList[files.gameLevelObjects[i].baseModelIndex].constantBuffer->GetGPUVirtualAddress());
			cmd->IASetVertexBuffers(0, 1, &UniqueObjectList[files.gameLevelObjects[i].baseModelIndex].vertexView);
			cmd->IASetIndexBuffer(&UniqueObjectList[files.gameLevelObjects[i].baseModelIndex].indexView);
			cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			for (size_t j = 0; j < UniqueObjectList[files.gameLevelObjects[i].baseModelIndex].meshCount; j++)
			{
				//Update World Matrix for each mesh
				UINT8* transferMemoryLocation;
				UniqueObjectList[files.gameLevelObjects[i].baseModelIndex].MeshDataList[j].worldMatrix = files.gameLevelObjects[i].pos;
				UniqueObjectList[files.gameLevelObjects[i].baseModelIndex].constantBuffer->Map(0, &CD3DX12_RANGE(0, 0),
					reinterpret_cast<void**>(&transferMemoryLocation));

				memcpy(transferMemoryLocation + sizeof(SCENE_DATA) + (sizeof(MESH_DATA) * j),
					&UniqueObjectList[files.gameLevelObjects[i].baseModelIndex].MeshDataList[j], sizeof(MESH_DATA));

				UniqueObjectList[files.gameLevelObjects[i].baseModelIndex].constantBuffer->Unmap(0, nullptr);
				cmd->SetGraphicsRootConstantBufferView(1, 
					UniqueObjectList[files.gameLevelObjects[i].baseModelIndex].constantBuffer->GetGPUVirtualAddress() +
					sizeof(SCENE_DATA) + (sizeof(MESH_DATA) * j));
				cmd->DrawIndexedInstanced(UniqueObjectList[files.gameLevelObjects[i].baseModelIndex].objects[j].indexCount, 1,
					UniqueObjectList[files.gameLevelObjects[i].baseModelIndex].objects[j].indexOffset, 0, 0);
			}
		}
		// release temp handles
		cmd->Release();
	}
	~Renderer()
	{
		// ComPtr will auto release so nothing to do here 
	}
};
