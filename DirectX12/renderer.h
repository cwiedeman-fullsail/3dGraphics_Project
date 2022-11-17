// minimalistic code to draw a single triangle, this is not part of the API.
// required for compiling shaders on the fly, consider pre-compiling instead

// Frustrum Culling adapted from https://www.braynzarsoft.net/viewtutorial/q16390-34-aabb-cpu-side-frustum-culling

#include "Model.h";
#include "FileIO.h"

struct Gamelevel
{
	std::string TXTname;
	std::string H2Bfolder;

};

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

//vector<Model> CompleteModelList;
//FileIO files;
//SCENE_DATA camerAndLights;
int currentLevel = 1;
vector<Model> CompleteModelList;
vector<D3D12_VIEWPORT> views;
vector<GW::MATH::GVECTORF>FrustumPlane;
bool culling = false;
bool canCollid = true;



// Creation, Rendering & Cleanup
class Renderer
{
#pragma region Declarations


	Gamelevel level1;
	Gamelevel level2;

	float volume = 0.005f;
	std::string TXTname;
	std::string H2Bfolder;
	bool collided = false;
	GW::MATH::GMATRIXF lastGoodPosition;

	GW::INPUT::GInput KBM;
	GW::INPUT::GController Control;
	GW::AUDIO::GAudio Sounds;
	GW::AUDIO::GMusic Music;
	GW::AUDIO::GSound SFX;

	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GDirectX12Surface d3d;
	GW::MATH::GMatrix Math;
	GW::MATH::GVector Math2;
	GW::MATH::GCollision Collision;

	// what we need at a minimum to draw a triangle
	Microsoft::WRL::ComPtr<ID3D12RootSignature>	rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState>	pipeline;
	D3D12_VIEWPORT mainView;
	D3D12_VIEWPORT miniView;


	//Matrixes
	GW::MATH::GMATRIXF viewM = GW::MATH::GIdentityMatrixF;
	GW::MATH::GMATRIXF projM = GW::MATH::GIdentityMatrixF;
	GW::MATH::GMATRIXF worldM = GW::MATH::GIdentityMatrixF;
	GW::MATH::GMATRIXF mapM = GW::MATH::GIdentityMatrixF;
	//directional light
	GW::MATH::GVECTORF light_direction = { -1.0f, -100.0f, 2.0f };
	GW::MATH::GVECTORF light_color = { 0.9f, 0.9f, 1.0f, 1.0f };

public:
	FileIO files;
	SCENE_DATA camerAndLights;
	SCENE_DATA miniMap;
	MESH_DATA boundingMesh;
	GW::MATH::GSPHEREF CameraCollider;

	std::vector<Gamelevel> levels;
	GW::MATH::GVECTORF eye = { 0 };
	GW::MATH::GVECTORF at = { 0 };
	GW::MATH::GVECTORF up = { 0 };
	GW::MATH::GVECTORF moveV;
	GW::MATH::GVECTORF LastMove;
	GW::MATH::GMATRIXF rotationM;
	float rotateX;
	float rotateY;
	float rotateZ;

	float mouseX;
	float mouseY;
	float duration = 0.0000000f;
	float speed = 0;
	float mSpeed = 20;
	float defaultSpeed = 100;
	bool pressed = false;
	float pressTimer = 0;
#pragma endregion

#pragma region Camera Controls
	void UpdateCamera(bool* _bounds, bool* _mMapShow)
	{
		KBM.Create(win);
		Control.Create();

		std::chrono::steady_clock::time_point _end(std::chrono::steady_clock::now());

		moveV = { 0 };

		rotationM = { 0 };
		rotateX = 0;
		rotateY = 0;
		rotateZ = 0;
		speed = defaultSpeed;
		GW::MATH::GMATRIXF view_copy = camerAndLights.viewMatrix;
		float RollLeft = 0;
		float RollRight = 0;
		float Up = 0;
		float down = 0;
		float left = 0;
		float right = 0;
		float forward = 0;
		float backward = 0;
		float boost = 0;
		float leftClick = 0;
		float level1Select = 0;
		float level2Select = 0;
		float cameraPreset1 = 0;
		float cameraPreset2 = 0;
		float cameraPreset3 = 0;
		float volUP = 0;
		float volDOWN = 0;
		float sFX = 0;
		float renderBounds = 0;
		float cull = 0;
		float miniMapShow = 0;
		float musicToggle = 0;
		float Ccollide = 0;

		bool connected = false;


		GW::GReturn result;
		Control.IsConnected(0, connected);

		if (connected)
		{
			Control.GetState(0, G_LEFT_SHOULDER_BTN, RollLeft);
			Control.GetState(0, G_RIGHT_SHOULDER_BTN, RollRight);
			Control.GetState(0, G_EAST_BTN, boost);
			if (RollLeft > 0)
			{
				rotateZ = -(speed * duration);
				//std::cout << "Z Key: " << Zf << "\n";
			};
			if (RollRight > 0)
			{
				rotateZ = (speed * duration);
				//std::cout << "C Key: " << Cf << "\n";
			};
			if (boost > 0)
			{
				speed = 50;
			}
		}
		KBM.GetState(G_KEY_Q, RollLeft);
		KBM.GetState(G_KEY_1, cameraPreset1);
		KBM.GetState(G_KEY_2, cameraPreset2);
		KBM.GetState(G_KEY_3, cameraPreset3);
		KBM.GetState(G_KEY_E, RollRight);
		KBM.GetState(G_KEY_F, sFX);
		KBM.GetState(G_KEY_R, renderBounds);
		KBM.GetState(G_KEY_C, cull);
		KBM.GetState(G_KEY_M, miniMapShow);
		KBM.GetState(G_KEY_P, musicToggle);
		KBM.GetState(G_KEY_NUMPAD_ADD, volUP);
		KBM.GetState(G_KEY_NUMPAD_SUBTRACT, volDOWN);
		KBM.GetState(G_KEY_LEFTSHIFT, boost);
		KBM.GetState(G_KEY_TAB, Ccollide);
		if (cameraPreset1 > 0)
		{
			eye = { 20.0f, 20.0f, -1.0f };
			at = { 0.1f, 0.1f, 0.1f };
			up = { 0.0f, 1.0f, 0.0f };
			Math.LookAtLHF(eye, at, up, miniMap.viewMatrix);

		}
		if (cameraPreset2 > 0)
		{
			eye = { 40.0f, 40.0f, 40.0f };
			at = { 0.1f, 0.1f, 0.1f };
			up = { 0.0f, 1.0f, 0.0f };
			Math.LookAtLHF(eye, at, up, miniMap.viewMatrix);
		}
		if (cameraPreset3 > 0)
		{
			eye = { 5.0f, 5.0f, 5.0f };
			at = { 0.1f, 0.1f, 0.1f };
			up = { 0.0f, 1.0f, 0.0f };
			Math.LookAtLHF(eye, at, up, miniMap.viewMatrix);
		}
		if (Ccollide > 0)
		{
			if (!pressed)
			{
				canCollid = !canCollid;
				pressed = true;
			}
		}
		if (volUP > 0)
		{
			volume += 0.0001f;
			Music.SetVolume(volume);
		}
		if (volDOWN > 0)
		{
			volume -= 0.0001f;
			Music.SetVolume(volume);
		}
		if (sFX > 0)
		{
			if (!pressed)
			{
				SFX.Create("../audio/Bonk.wav", Sounds, 0.1f);
				SFX.Play();
				pressed = true;
			}
		}

		if (renderBounds > 0)
		{
			if (!pressed)
			{
				*_bounds = !(*_bounds);
				pressed = true;
			}
		}
		if (cull > 0)
		{
			if (!pressed)
			{
				culling = !culling;
				pressed = true;
			}
		}

		if (RollLeft > 0)
		{
			rotateZ = -(speed * duration) / 2;
			//std::cout << "Z Key: " << Zf << "\n";
		};
		if (RollRight > 0)
		{
			rotateZ = (speed * duration) / 2;
			//std::cout << "C Key: " << Cf << "\n";
		};
		if (boost > 0)
		{
			speed = 2000;
		}
		if (miniMapShow > 0)
		{
			if (!pressed)
			{
				*_mMapShow = !(*_mMapShow);
				pressed = true;
			}
		}
		if (musicToggle > 0)
		{
			if (!pressed)
			{
				bool isPlaying = true;
				Music.isPlaying(isPlaying);
				if (isPlaying)
				{
					Music.Pause();
				}
				else
				{
					Music.Resume();
				}
				pressed = true;
			}
		}
		KBM.GetState(G_KEY_SPACE, Up);
		KBM.GetState(G_KEY_LEFTCONTROL, down);
		KBM.GetState(G_KEY_A, left);
		KBM.GetState(G_KEY_D, right);
		KBM.GetState(G_KEY_W, forward);
		KBM.GetState(G_KEY_S, backward);
		KBM.GetState(G_BUTTON_LEFT, leftClick);


		if (backward > 0)
		{
			moveV.z = speed * duration;
		};
		if (forward > 0)
		{
			moveV.z = -speed * duration;
		};
		if (right > 0)
		{
			moveV.x = -speed * duration;
		};
		if (left > 0)
		{
			moveV.x = speed * duration;
		};
		if (down > 0)
		{
			moveV.y = speed * duration;
		};
		if (Up > 0)
		{
			moveV.y = -speed * duration;
		};

		result = KBM.GetMouseDelta(mouseX, mouseY);
		if (G_PASS(result) && result != GW::GReturn::REDUNDANT && leftClick > 0)
		{
			//std::cout << mouseX << " " << mouseY << "\n";
			rotateX = (-mouseX) * mSpeed * duration;
			rotateY = (-mouseY) * mSpeed * duration;
		};

		result = Control.GetState(0, G_RX_AXIS, mouseX);
		if (G_PASS(result) && result != GW::GReturn::REDUNDANT && (mouseX > 0.05f || mouseX < -0.05f))
		{
			rotateX = (-mouseX) * mSpeed * duration;
		};
		result = Control.GetState(0, G_RY_AXIS, mouseY);
		if (G_PASS(result) && result != GW::GReturn::REDUNDANT && (mouseY > 0.05f || mouseY < -0.05f))
		{
			rotateY = (-mouseY) * mSpeed * duration;
		};
		result = Control.GetState(0, G_LY_AXIS, forward);
		if (G_PASS(result) && result != GW::GReturn::REDUNDANT && (forward > 0.05f || forward < -0.05f))
		{
			moveV.z = (-speed * duration) * forward;
		};
		result = Control.GetState(0, G_LX_AXIS, left);
		if (G_PASS(result) && result != GW::GReturn::REDUNDANT && (left > 0.05f || left < -0.05f))
		{
			moveV.x = (-speed * duration) * left;
		};
		result = Control.GetState(0, G_LEFT_TRIGGER_AXIS, down);
		if (G_PASS(result) && result != GW::GReturn::REDUNDANT && down > 0.05f)
		{
			moveV.y = (speed * duration) * down;
		};
		result = Control.GetState(0, G_RIGHT_TRIGGER_AXIS, Up);
		if (G_PASS(result) && result != GW::GReturn::REDUNDANT && Up > 0.05f)
		{
			moveV.y = (-speed * duration) * Up;
		};

		Math.RotationYawPitchRollF(0, 0, rotateZ, rotationM);
		Math.RotateYLocalF(rotationM, rotateX, rotationM);
		Math.RotateXGlobalF(rotationM, rotateY, rotationM);
		if (canCollid)
		{

			if (!collided)
			{
				Math.TranslateGlobalF(view_copy, moveV, view_copy);
				LastMove = moveV;
			}
			else
			{
				if (!pressed)
				{
					SFX.Create("../audio/Bonk.wav", Sounds, 0.1f);
					SFX.Play();
					pressed = true;
				}
				if (LastMove.x > 0)
				{
					LastMove.x = -LastMove.x - 0.1f;
				}
				else if (LastMove.x < 0)
				{
					LastMove.x = -LastMove.x + 0.1f;
				}
				if (LastMove.y > 0)
				{
					LastMove.y = -LastMove.y - 0.1f;
				}
				else if (LastMove.y < 0)
				{
					LastMove.y = -LastMove.y + 0.1f;
				}
				if (LastMove.z > 0)
				{
					LastMove.z = -LastMove.z - 0.1f;
				}
				else if (LastMove.z < 0)
				{
					LastMove.z = -LastMove.z + 0.1f;
				}
				Math.TranslateGlobalF(view_copy, LastMove, view_copy);
				collided = false;
			}
		}
		else
		{
			Math.TranslateGlobalF(view_copy, moveV, view_copy);
			LastMove = moveV;
		}
		Math.MultiplyMatrixF(view_copy, rotationM, camerAndLights.viewMatrix);
		//camera position for specular lights
		GW::MATH::GVECTORF temp2;
		Math.GetTranslationF(camerAndLights.viewMatrix, temp2);
		camerAndLights.camPOS = temp2;
		std::chrono::steady_clock::time_point _start(std::chrono::steady_clock::now());
		duration = std::chrono::duration_cast<std::chrono::duration<float>>(_start - _end).count();
		if (pressed)
		{
			pressTimer += duration;
			if (pressTimer > 0.001f)
			{
				pressed = false;
				pressTimer = 0;
			}
		}
		if (duration < 0.00001)
		{
			duration = 0.0001;
		}
		//std::cout << duration << '\n';
	}
#pragma endregion

#pragma region Frustum Culling


	std::vector<GW::MATH::GVECTORF> getFrustumPlanes(GW::MATH::GMATRIXF& viewProj)
	{
		std::vector<GW::MATH::GVECTORF> Frustum(6);

		Frustum[0].x = viewProj.row1.w + viewProj.row1.x;
		Frustum[0].y = viewProj.row2.w + viewProj.row2.x;
		Frustum[0].z = viewProj.row3.w + viewProj.row3.x;
		Frustum[0].w = viewProj.row4.w + viewProj.row4.x;

		Frustum[1].x = viewProj.row1.w - viewProj.row1.x;
		Frustum[1].y = viewProj.row2.w - viewProj.row2.x;
		Frustum[1].z = viewProj.row3.w - viewProj.row3.x;
		Frustum[1].w = viewProj.row4.w - viewProj.row4.x;

		Frustum[2].x = viewProj.row1.w - viewProj.row1.y;
		Frustum[2].y = viewProj.row2.w - viewProj.row2.y;
		Frustum[2].z = viewProj.row3.w - viewProj.row3.y;
		Frustum[2].w = viewProj.row4.w - viewProj.row4.y;

		Frustum[3].x = viewProj.row1.w + viewProj.row1.y;
		Frustum[3].y = viewProj.row2.w + viewProj.row2.y;
		Frustum[3].z = viewProj.row3.w + viewProj.row3.y;
		Frustum[3].w = viewProj.row4.w + viewProj.row4.y;

		Frustum[4].x = viewProj.row1.z;
		Frustum[4].y = viewProj.row2.z;
		Frustum[4].z = viewProj.row3.z;
		Frustum[4].w = viewProj.row4.z;

		Frustum[5].x = viewProj.row1.w + viewProj.row1.z;
		Frustum[5].y = viewProj.row2.w + viewProj.row2.z;
		Frustum[5].z = viewProj.row3.w + viewProj.row3.z;
		Frustum[5].w = viewProj.row4.w + viewProj.row4.z;

		for (int i = 0; i < 6; ++i)
		{

			float length = sqrt((Frustum[i].x * Frustum[i].x) + (Frustum[i].y * Frustum[i].y) + (Frustum[i].z * Frustum[i].z));
			Frustum[i].x /= length;
			Frustum[i].y /= length;
			Frustum[i].z /= length;
			Frustum[i].w /= length;
		}

		return Frustum;
	}
	bool isOnScreen(std::vector<GW::MATH::GVECTORF>& frustumPlanes, Model _m)
	{
		// Loop through each frustum plane
		for (int planeID = 0; planeID < 6; ++planeID)
		{
			GW::MATH::GVECTORF planeNormal = { frustumPlanes[planeID].x, frustumPlanes[planeID].y, frustumPlanes[planeID].z, 0.0f };
			float planeConstant = frustumPlanes[planeID].w;

			GW::MATH::GVECTORF axisVert;

			GW::MATH::GVECTORF pos;
			Math.GetTranslationF(_m.worldMatrix, pos);
			if (frustumPlanes[planeID].x < 0.0f)
				axisVert.x = _m.AABB.min.x + pos.x;
			else
				axisVert.x = _m.AABB.max.x + pos.x;
			if (frustumPlanes[planeID].y < 0.0f)
				axisVert.y = _m.AABB.min.y + pos.y;
			else
				axisVert.y = _m.AABB.max.y + pos.y;
			if (frustumPlanes[planeID].z < 0.0f)
				axisVert.z = _m.AABB.min.z + pos.z;
			else
				axisVert.z = _m.AABB.max.z + pos.z;
			float result;
			Math2.DotF(planeNormal, axisVert, result);
			if (result + planeConstant < 0.0f)
			{
				return false;
			}
		}
		return true;
	}
#pragma endregion

	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GDirectX12Surface _d3d)
	{

		//set h2b files read folder
		level1.TXTname = "../assets/GameLevel.txt";
		level1.H2Bfolder = "assets";
		//level2.TXTname = "../Test/GameLevel2.txt";
		//level2.H2Bfolder = "Test";


		levels.push_back(level1);
		//levels.push_back(level2);

		string gameLevelPath = levels[currentLevel - 1].TXTname;
		files.h2BLocationFolder = levels[currentLevel - 1].H2Bfolder;
		Math.Create();
		KBM.Create(win);
		win = _win;
		d3d = _d3d;
		ID3D12Device* creator;
		d3d.GetDevice((void**)&creator);
		KBM.Create(win);
		Collision.Create();

#pragma region Matricies and Scene Data
		//Create Matrixes
		//view
		GW::MATH::GVECTORF eye = { 5.0f, 5.0f, -10.0f };
		GW::MATH::GVECTORF at = { -30.0f,0.0f,1.0f };
		GW::MATH::GVECTORF up = { 0.0f, 1.0f, 0.0f };
		Math.LookAtLHF(eye, at, up, viewM);

		eye = { 20.0f, 20.0f, -1.0f };
		at = { 0.1f, 0.1f, 0.1f };
		up = { 0.0f, 1.0f, 0.0f };
		Math.LookAtLHF(eye, at, up, mapM);

		//projection
		float aspect;
		d3d.GetAspectRatio(aspect);
		Math.ProjectionDirectXLHF(G_DEGREE_TO_RADIAN_F(65), aspect, 0.1f, 200, projM);

		GW::MATH::GVector::NormalizeF(light_direction, camerAndLights.sunDirection);
		//camerAndLights.sunDirection = light_direction;
		camerAndLights.sunColor = light_color;
		camerAndLights.viewMatrix = viewM;
		camerAndLights.projectionMatrix = projM;
		camerAndLights.sumAmb = { 0.25f, 0.25f, 0.35f, 1.0f };

		GW::MATH::GVector::NormalizeF(light_direction, miniMap.sunDirection);
		//camerAndLights.sunDirection = light_direction;
		miniMap.sunColor = light_color;
		miniMap.viewMatrix = mapM;
		miniMap.projectionMatrix = projM;
		miniMap.sumAmb = { 0.55f, 0.25f, 0.25f, 1.0f };


		//caculate Camera Position
		GW::MATH::GMATRIXF temp;
		Math.InverseF(viewM, temp);
		GW::MATH::GVECTORF temp2;
		Math.GetTranslationF(temp, temp2);
		camerAndLights.camPOS = temp2;

		GW::MATH::GSPHEREF CameraCollider = { camerAndLights.camPOS.x,camerAndLights.camPOS.y,camerAndLights.camPOS.z, 0.5f };
#pragma endregion

		DXGI_SWAP_CHAIN_DESC sw_desc;
		Microsoft::WRL::ComPtr <IDXGISwapChain4> test = nullptr;
		d3d.GetSwapchain4((void**)&test);
		test->GetDesc(&sw_desc);
		int frames = sw_desc.BufferCount;


		//read files
		files.ReadFile(gameLevelPath);
		files.ReadH2B();

#pragma region Populate Model
		for (size_t i = 0; i < files.meshAndMaterialData.size(); i++)
		{
			Model M;
			M.modelName = files.gameLevelObjects[i].name;
			M.modelType = files.gameLevelObjects[i].type;
			M.vCount = files.meshAndMaterialData[i].vertexCount;
			M.iCount = files.meshAndMaterialData[i].indexCount;
			M.matCount = files.meshAndMaterialData[i].materialCount;
			M.meshCount = files.meshAndMaterialData[i].meshCount;
			M.worldMatrix = files.gameLevelObjects[i].pos;
			M.createConstantBuffer(creator, frames);
			M.createBoundingConstantBuffer(creator, frames);
			for (size_t j = 0; j < files.meshAndMaterialData[i].materialCount; j++)
			{
				M.buildMateralAttributeList(
					VEC3{ files.meshAndMaterialData[i].materials[j].attrib.Kd.x, files.meshAndMaterialData[i].materials[j].attrib.Kd.y,files.meshAndMaterialData[i].materials[j].attrib.Kd.z },
					files.meshAndMaterialData[i].materials[j].attrib.d,
					VEC3{ files.meshAndMaterialData[i].materials[j].attrib.Ks.x,files.meshAndMaterialData[i].materials[j].attrib.Ks.y,files.meshAndMaterialData[i].materials[j].attrib.Ks.z },
					files.meshAndMaterialData[i].materials[j].attrib.Ns,
					VEC3{ files.meshAndMaterialData[i].materials[j].attrib.Ka.x,files.meshAndMaterialData[i].materials[j].attrib.Ka.y,files.meshAndMaterialData[i].materials[j].attrib.Ka.z },
					files.meshAndMaterialData[i].materials[j].attrib.sharpness,
					VEC3{ files.meshAndMaterialData[i].materials[j].attrib.Tf.x, files.meshAndMaterialData[i].materials[j].attrib.Tf.y, files.meshAndMaterialData[i].materials[j].attrib.Tf.z },
					files.meshAndMaterialData[i].materials[j].attrib.Ni,
					VEC3{ files.meshAndMaterialData[i].materials[j].attrib.Ke.x, files.meshAndMaterialData[i].materials[j].attrib.Ke.y,files.meshAndMaterialData[i].materials[j].attrib.Ke.z },
					files.meshAndMaterialData[i].materials[j].attrib.illum);
				M.buildMeshList(files.meshAndMaterialData[i].meshes[j].drawInfo.indexCount, files.meshAndMaterialData[i].meshes[j].drawInfo.indexOffset);
				M.CamandLight = camerAndLights;
				M.MiniMap = miniMap;
				MESH_DATA mesh;
				mesh.worldMatrix = files.gameLevelObjects[i].pos;
				mesh.material = M.materials[j];
				M.meshAndMaterialDataList.push_back(mesh);
				M.loadMaterialsToGPU(&M.meshAndMaterialDataList[j], j);
				M.createDescriptorHeap(creator);
				M.createCBView();
				M.createBoundingCBView();
			}
			M.boundingMesh.worldMatrix = files.gameLevelObjects[i].pos;
			M.loadBoundingMaterialsToGPU(&M.boundingMesh);
			// Create Vertex Buffer
			for (size_t j = 0; j < files.meshAndMaterialData[i].vertices.size(); j++)
			{
				Vertex v;
				v.pos.x = files.meshAndMaterialData[i].vertices[j].pos.x;
				v.pos.y = files.meshAndMaterialData[i].vertices[j].pos.y;
				v.pos.z = files.meshAndMaterialData[i].vertices[j].pos.z;
				v.pos.w = 1;
				v.uvw.x = files.meshAndMaterialData[i].vertices[j].uvw.x;
				v.uvw.y = files.meshAndMaterialData[i].vertices[j].uvw.y;
				v.uvw.z = files.meshAndMaterialData[i].vertices[j].uvw.z;
				v.uvw.w = 1;
				v.nrm.x = files.meshAndMaterialData[i].vertices[j].nrm.x;
				v.nrm.y = files.meshAndMaterialData[i].vertices[j].nrm.y;
				v.nrm.z = files.meshAndMaterialData[i].vertices[j].nrm.z;
				M.addToVertList(v);
			}
			M.AABB = M.CreateAABB(M.vertList);
			M.createVertexBuffer(creator);
			M.createBoundingVertexBuffer(creator);

			//Create Index Buffer
			for (size_t j = 0; j < files.meshAndMaterialData[i].indices.size(); j++)
			{
				M.addToIndexList(files.meshAndMaterialData[i].indices[j]);
			}


			M.createIndexBuffer(creator);
			M.createBoundingIndexBuffer(creator);

			CompleteModelList.push_back(M);
		}
#pragma endregion

#pragma region Create Shaders

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
#pragma endregion

#pragma region Create InputLayout and Pipeline


		// Create Input Layout
		D3D12_INPUT_ELEMENT_DESC format[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
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
		creator->CreateGraphicsPipelineState(&psDesc, IID_PPV_ARGS(pipeline.GetAddressOf()));
		// free temporary handle
		creator->Release();
#pragma endregion

#pragma region Set up Viewports and music


		UINT height;
		UINT width;
		win.GetHeight(height);
		win.GetWidth(width);
		mainView.Height = height;
		mainView.Width = width;
		mainView.TopLeftX = 0;
		mainView.TopLeftY = 0;
		mainView.MinDepth = 0;
		mainView.MaxDepth = 1;
		views.push_back(mainView);
		miniView.Height = height / 4;
		miniView.Width = width / 4;
		miniView.TopLeftX = 20;
		miniView.TopLeftY = 10;
		miniView.MinDepth = 0;
		miniView.MaxDepth = 0.001;
		views.push_back(miniView);
		Sounds.Create();
		Music.Create("../audio/music1.wav", Sounds, volume);
		Music.Play();


	}
#pragma endregion

#pragma region Render main Window

	void Render()
	{
		GW::MATH::GMATRIXF cam;
		GW::MATH::GVECTORF cam_pos;
		Math.InverseF(camerAndLights.viewMatrix, cam);
		Math.GetTranslationF(cam, cam_pos);
		CameraCollider = { cam_pos.x,cam_pos.y,cam_pos.z, 1.0f };
		GW::GReturn result;
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
		GW::MATH::GMATRIXF VP;
		Math.MultiplyMatrixF(camerAndLights.viewMatrix, camerAndLights.projectionMatrix, VP);
		// now we can draw
		for (size_t i = 0; i < CompleteModelList.size(); i++)
		{
			GW::MATH::GCollision::GCollisionCheck check;
			GW::MATH::GVECTORF cameraContactPoint, modelContactPoint, Direction;
			float distance;
			GW::MATH::GAABBCEF AABBC;
			GW::MATH::GVECTORF min = CompleteModelList[i].AABB.min; min.w = 1;
			GW::MATH::GVECTORF max = CompleteModelList[i].AABB.max; max.w = 1;
			Math.VectorXMatrixF(files.gameLevelObjects[i].pos, min, min);
			Math.VectorXMatrixF(files.gameLevelObjects[i].pos, max, max);
			if (min.x > max.x)
			{
				swap(min.x, max.x);
			}
			if (min.y > max.y)
			{
				swap(min.y, max.y);
			}
			if (min.z > max.z)
			{
				swap(min.z, max.z);
			}
			if (min.w > max.w)
			{
				swap(min.w, max.w);
			}
			GW::MATH::GAABBMMF AABBMM = { min, max };
			Collision.ConvertAABBMMToAABBCEF(AABBMM, AABBC);
			result = Collision.IntersectSphereToAABBF(CameraCollider, AABBC, check, cameraContactPoint, modelContactPoint, Direction, distance);
			if (check == GW::MATH::GCollision::GCollisionCheck::COLLISION)
			{
				//int hold = 0;
				//std::cout << "COLLISION with " << CompleteModelList[i].modelName << "!\n";
				collided = true;
			}
			if (CompleteModelList[i].modelType != "MESH")
			{
				continue;
			}
			cmd->SetDescriptorHeaps(1, CompleteModelList[i].dHeap.GetAddressOf());
			cmd->SetGraphicsRootConstantBufferView(0, CompleteModelList[i].constantBuffer->GetGPUVirtualAddress());
			cmd->IASetVertexBuffers(0, 1, &CompleteModelList[i].vertexView);
			cmd->IASetIndexBuffer(&CompleteModelList[i].indexView);
			cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			for (size_t j = 0; j < CompleteModelList[i].meshCount; j++)
			{
				//Update World Matrix for each mesh
				UINT8* transferMemoryLocation;
				CompleteModelList[i].meshAndMaterialDataList[j].worldMatrix = files.gameLevelObjects[i].pos;
				CompleteModelList[i].constantBuffer->Map(0, &CD3DX12_RANGE(0, 0),
					reinterpret_cast<void**>(&transferMemoryLocation));
				memcpy(transferMemoryLocation, &camerAndLights, sizeof(SCENE_DATA));
				memcpy(transferMemoryLocation + sizeof(SCENE_DATA), &miniMap, sizeof(SCENE_DATA));
				memcpy(transferMemoryLocation + (sizeof(SCENE_DATA) * 2) + (sizeof(MESH_DATA) * j),
					&CompleteModelList[i].meshAndMaterialDataList[j], sizeof(MESH_DATA));
				CompleteModelList[i].constantBuffer->Unmap(0, nullptr);

				cmd->SetGraphicsRootConstantBufferView(1,
					CompleteModelList[i].constantBuffer->GetGPUVirtualAddress() +
					(sizeof(SCENE_DATA) * 2) + (sizeof(MESH_DATA) * j));

				cmd->RSSetViewports(1, &mainView);

				if (isOnScreen(getFrustumPlanes(VP), CompleteModelList[i]))
				{
					cmd->DrawIndexedInstanced(CompleteModelList[i].objects[j].indexCount, 1,
						CompleteModelList[i].objects[j].indexOffset, 0, 0);
				}
			}
		}



		// release temp handles
		cmd->Release();
	}
#pragma endregion

#pragma region Render Mini Map


	void RenderMiniMap()
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
		GW::MATH::GMATRIXF VP;
		Math.MultiplyMatrixF(camerAndLights.viewMatrix, camerAndLights.projectionMatrix, VP);
		// now we can draw
		for (size_t i = 0; i < CompleteModelList.size(); i++)
		{
			if (CompleteModelList[i].modelType != "MESH")
			{
				continue;
			}
			cmd->SetDescriptorHeaps(1, CompleteModelList[i].dHeap.GetAddressOf());
			cmd->SetGraphicsRootConstantBufferView(0, CompleteModelList[i].constantBuffer->GetGPUVirtualAddress() + sizeof(SCENE_DATA));
			cmd->IASetVertexBuffers(0, 1, &CompleteModelList[i].vertexView);
			cmd->IASetIndexBuffer(&CompleteModelList[i].indexView);
			cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			for (size_t j = 0; j < CompleteModelList[i].meshCount; j++)
			{
				//Update World Matrix for each mesh


				cmd->SetGraphicsRootConstantBufferView(1,
					CompleteModelList[i].constantBuffer->GetGPUVirtualAddress() +
					(sizeof(SCENE_DATA) * 2) + (sizeof(MESH_DATA) * j));

				cmd->RSSetViewports(1, &miniView);
				if (culling)
				{
					if (isOnScreen(getFrustumPlanes(VP), CompleteModelList[i]))
					{
						cmd->DrawIndexedInstanced(CompleteModelList[i].objects[j].indexCount, 1,
							CompleteModelList[i].objects[j].indexOffset, 0, 0);
					}
				}
				else
				{
					cmd->DrawIndexedInstanced(CompleteModelList[i].objects[j].indexCount, 1,
						CompleteModelList[i].objects[j].indexOffset, 0, 0);
				}
			}
		}
		// release temp handles
		cmd->Release();
	}
#pragma endregion

#pragma region Render Boundries


	void RenderBounds()
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
		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		GW::MATH::GMATRIXF VP;
		Math.MultiplyMatrixF(camerAndLights.viewMatrix, camerAndLights.projectionMatrix, VP);
		// now we can draw
		for (size_t i = 0; i < CompleteModelList.size(); i++)
		{
			if (CompleteModelList[i].modelType != "MESH")
			{
				continue;
			}
			cmd->SetGraphicsRootConstantBufferView(0, CompleteModelList[i].BoundingconstantBuffer->GetGPUVirtualAddress());
			cmd->SetDescriptorHeaps(1, CompleteModelList[i].dHeap.GetAddressOf());
			cmd->IASetIndexBuffer(&CompleteModelList[i].boundingindexView);
			cmd->IASetVertexBuffers(0, 1, &CompleteModelList[i].boundingvertexView);
			UINT8* transferMemoryLocation;
			CompleteModelList[i].BoundingconstantBuffer->Map(0, &CD3DX12_RANGE(0, 0),
				reinterpret_cast<void**>(&transferMemoryLocation));
			memcpy(transferMemoryLocation, &camerAndLights, sizeof(SCENE_DATA));
			//CompleteModelList[i].boundingMesh.worldMatrix = GW::MATH::GIdentityMatrixF;
			memcpy(transferMemoryLocation + (sizeof(SCENE_DATA) * 2),
				&CompleteModelList[i].boundingMesh, sizeof(MESH_DATA));
			CompleteModelList[i].BoundingconstantBuffer->Unmap(0, nullptr);
			cmd->SetGraphicsRootConstantBufferView(1,
				CompleteModelList[i].BoundingconstantBuffer->GetGPUVirtualAddress() +
				(sizeof(SCENE_DATA) * 2));
			cmd->RSSetViewports(1, &mainView);
			//if (cullAABB(getFrustumPlanes(VP), CompleteModelList[i]))
			//{
			cmd->DrawIndexedInstanced(CompleteModelList[i].boundIList.size(), 1, 0, 0, 0);
			//}
		}
		// release temp handles
		cmd->Release();
	}
#pragma endregion
	~Renderer()
	{
		// ComPtr will auto release so nothing to do here 
	}
};
