#pragma pack_matrix(row_major)

struct OBJ_ATTRIBUTES
{
    float3 Kd; // diffuse reflectivity
    float d; // dissolve (transparency) 
    float3 Ks; // specular reflectivity
    float Ns; // specular exponent
    float3 Ka; // ambient reflectivity
    float sharpness; // local reflection map sharpness
    float3 Tf; // transmission filter
    float Ni; // optical density (index of refraction)
    float3 Ke; // emissive reflectivity
    uint illum; // illumination model
};

struct SCENE_DATA
{
    float4 sunDirection, sunColor;
    float4x4 viewMatrix, projectionMatrix;
    float4 sunAmb;
    float4 camPOS;
    float4 padding[4];
};

struct MESH_DATA
{
    float4x4 worldMatrix;
    OBJ_ATTRIBUTES material;
    unsigned int padding[28];
};

struct inVertex
{
    float4 pos : POSITION;
    float4 uvw : TEXCOORD;
    float3 nrm : NORMAL;
};
struct outVertex
{
    float4 pos : SV_POSITION;
    float4 uvw : TEXCOORD;
    float3 nrm : NORMAL;
};
struct OUT_TO_RASTERIZER
{
    float4 posH : SV_Position;
    float3 nrmW : NORMAL;
    float3 posW : WORLD;
    float4 uvw : TEXCOORD;
    //float4 camPOS :CAMERA;
};

ConstantBuffer<SCENE_DATA> cameraAndLights : register(b0, Space0);
ConstantBuffer<MESH_DATA> meshInfo : register(b1, Space0);

OUT_TO_RASTERIZER main(inVertex inputVertex)
{
    OUT_TO_RASTERIZER output;
    output.posH = mul(inputVertex.pos, meshInfo.worldMatrix);
    output.posH = mul(output.posH, cameraAndLights.viewMatrix);
    output.posH = mul(output.posH, cameraAndLights.projectionMatrix);
    output.nrmW = mul(float4(inputVertex.nrm, 0), meshInfo.worldMatrix);
    output.posW = mul(inputVertex.pos, meshInfo.worldMatrix);
    output.uvw = inputVertex.uvw;
    return output;
}