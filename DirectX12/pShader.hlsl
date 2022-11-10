
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

ConstantBuffer<SCENE_DATA> cameraAndLights : register(b0, Space0);
ConstantBuffer<MESH_DATA> meshInfo : register(b1, Space0);

float4 main(float4 posH : SV_Position, float3 nrmW : NORMAL, float3 posW : WORLD) : SV_TARGET
{
    float lightRatio = saturate(dot(normalize(-cameraAndLights.sunDirection.xyz), normalize(nrmW)));
    float4 indirect = cameraAndLights.sunAmb * cameraAndLights.sunColor * float4(meshInfo.material.Ka, 1);
    float4 direct = saturate(lightRatio + indirect) * float4(meshInfo.material.Kd, 1);
    
    float3 viewDir = normalize(cameraAndLights.camPOS.xyz - posW);
    float3 halfVector = normalize(-cameraAndLights.sunDirection.rgb + viewDir);
    float intensity = max(pow(saturate(dot(normalize(nrmW), halfVector)), meshInfo.material.Ns), 0);
    float4 reflected = intensity * cameraAndLights.sunColor * float4(meshInfo.material.Ks, 1);
       
    return saturate(direct + reflected);
}