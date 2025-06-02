// -----------------------------
// -- Hazel Engine PBR shader --
// -----------------------------
// Note: this shader is still very much in progress. There are likely many bugs and future additions that will go in.
//       Currently heavily updated. 
//
// References upon which this is based:
// - Unreal Engine 4 PBR notes (https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf)
// - Frostbite's SIGGRAPH 2014 paper (https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/)
// - Michał Siejak's PBR project (https://github.com/Nadrin)
// - My implementation from years ago in the Sparky engine (https://github.com/TheCherno/Sparky)

#type vertex
#version 430 core

// 顶点输入属性
layout(location = 0) in vec3 a_Position;   // 顶点位置
layout(location = 1) in vec3 a_Normal;     // 法线
layout(location = 2) in vec3 a_Tangent;    // 切线
layout(location = 3) in vec3 a_Binormal;   // 副法线
layout(location = 4) in vec2 a_TexCoord;   // 纹理坐标

layout(location = 5) in ivec4 a_BoneIndices;   // 骨骼索引
layout(location = 6) in vec4 a_BoneWeights;    // 骨骼权重

// Uniforms
uniform mat4 u_ViewProjectionMatrix;       // 视图投影矩阵
uniform mat4 u_ModelMatrix;                // 模型矩阵

const int MAX_BONES = 100;
uniform mat4 u_BoneTransforms[100];        // 骨骼变换矩阵数组

// 顶点着色器输出结构体
out VertexOutput
{
	vec3 WorldPosition;    // 世界空间位置
    vec3 Normal;           // 法线
	vec2 TexCoord;         // 纹理坐标
	mat3 WorldNormals;     // 世界空间法线矩阵（TBN）
	vec3 Binormal;         // 世界空间副法线
} vs_Output;

void main()
{
	// 骨骼动画变换（线性混合骨骼矩阵）
	mat4 boneTransform = u_BoneTransforms[a_BoneIndices[0]] * a_BoneWeights[0];
    boneTransform += u_BoneTransforms[a_BoneIndices[1]] * a_BoneWeights[1];
    boneTransform += u_BoneTransforms[a_BoneIndices[2]] * a_BoneWeights[2];
    boneTransform += u_BoneTransforms[a_BoneIndices[3]] * a_BoneWeights[3];

	vec4 localPosition = boneTransform * vec4(a_Position, 1.0);

	// 计算世界空间位置和法线
	vs_Output.WorldPosition = vec3(u_ModelMatrix * boneTransform * vec4(a_Position, 1.0));
    vs_Output.Normal = mat3(boneTransform) * a_Normal;
	// 纹理坐标，V 轴翻转
	vs_Output.TexCoord = vec2(a_TexCoord.x, 1.0 - a_TexCoord.y);
	// 世界空间 TBN 矩阵
	vs_Output.WorldNormals = mat3(u_ModelMatrix) * mat3(a_Tangent, a_Binormal, a_Normal);
	vs_Output.Binormal = mat3(boneTransform) * a_Binormal;

	// 输出最终裁剪空间位置
	gl_Position = u_ViewProjectionMatrix * u_ModelMatrix * localPosition;
}

#type fragment
#version 430 core

// 常量定义
const float PI = 3.141592;
const float Epsilon = 0.00001;
const int LightCount = 1;

// 非金属的菲涅尔常量
const vec3 Fdielectric = vec3(0.04);

// 光源结构体
struct Light {
	vec3 Direction; // 光源方向
	vec3 Radiance;  // 光源辐射度
};

// 顶点着色器输出
in VertexOutput
{
	vec3 WorldPosition;
    vec3 Normal;
	vec2 TexCoord;
	mat3 WorldNormals;
	vec3 Binormal;
} vs_Input;

// 片元输出
layout(location=0) out vec4 color;

// Uniforms
uniform Light lights;                 // 场景主光源
uniform vec3 u_CameraPosition;        // 相机世界空间位置

// PBR相关纹理输入
uniform sampler2D u_AlbedoTexture;    // 漫反射贴图
uniform sampler2D u_NormalTexture;    // 法线贴图
uniform sampler2D u_MetalnessTexture; // 金属度贴图
uniform sampler2D u_RoughnessTexture; // 粗糙度贴图

// 环境贴图
uniform samplerCube u_EnvRadianceTex;   // 环境辐射度贴图（高光）
uniform samplerCube u_EnvIrradianceTex; // 环境辐照度贴图（漫反射）

// BRDF查找表
uniform sampler2D u_BRDFLUTTexture;     // BRDF LUT 贴图

// 材质参数
uniform vec3 u_AlbedoColor;             // 漫反射颜色
uniform float u_Metalness;              // 金属度
uniform float u_Roughness;              // 粗糙度

// 环境贴图旋转
uniform float u_EnvMapRotation;         // 环境贴图旋转角度

// 各种功能开关
uniform float u_RadiancePrefilter;      // 是否使用预过滤环境贴图
uniform float u_AlbedoTexToggle;        // 是否启用漫反射贴图
uniform float u_NormalTexToggle;        // 是否启用法线贴图
uniform float u_MetalnessTexToggle;     // 是否启用金属度贴图
uniform float u_RoughnessTexToggle;     // 是否启用粗糙度贴图

// PBR参数结构体
struct PBRParameters
{
	vec3 Albedo;      // 漫反射颜色
	float Roughness;  // 粗糙度
	float Metalness;  // 金属度

	vec3 Normal;      // 法线
	vec3 View;        // 视线方向
	float NdotV;      // 法线与视线夹角余弦
};

PBRParameters m_Params;

// GGX/Towbridge-Reitz 法线分布函数
float ndfGGX(float cosLh, float roughness)
{
	float alpha = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

// Schlick-GGX 单项式
float gaSchlickG1(float cosTheta, float k)
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX 几何遮蔽函数（Smith方法）
float gaSchlickGGX(float cosLi, float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0; // Epic推荐的粗糙度重映射
	return gaSchlickG1(cosLi, k) * gaSchlickG1(NdotV, k);
}

// Schlick-GGX 几何项（单方向）
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

// Smith几何项（双方向）
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Schlick 菲涅尔近似
vec3 fresnelSchlick(vec3 F0, float cosTheta)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// 粗糙度相关的Schlick菲涅尔近似
vec3 fresnelSchlickRoughness(vec3 F0, float cosTheta, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
} 

// -------------------- 环境贴图重要性采样相关函数（用于IBL） --------------------

// Van der Corput序列，用于Hammersley采样
float RadicalInverse_VdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

// Hammersley采样点生成
vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

// GGX分布重要性采样
vec3 ImportanceSampleGGX(vec2 Xi, float Roughness, vec3 N)
{
	float a = Roughness * Roughness;
	float Phi = 2 * PI * Xi.x;
	float CosTheta = sqrt( (1 - Xi.y) / ( 1 + (a*a - 1) * Xi.y ) );
	float SinTheta = sqrt( 1 - CosTheta * CosTheta );

	vec3 H;
	H.x = SinTheta * cos( Phi );
	H.y = SinTheta * sin( Phi );
	H.z = CosTheta;

	vec3 UpVector = abs(N.z) < 0.999 ? vec3(0,0,1) : vec3(1,0,0);
	vec3 TangentX = normalize( cross( UpVector, N ) );
	vec3 TangentY = cross( N, TangentX );

	return TangentX * H.x + TangentY * H.y + N * H.z;
}

float TotalWeight = 0.0;

// 环境贴图预过滤（用于高光反射IBL，性能较低，建议离线生成）
vec3 PrefilterEnvMap(float Roughness, vec3 R)
{
	vec3 N = R;
	vec3 V = R;
	vec3 PrefilteredColor = vec3(0.0);
	int NumSamples = 1024;
	for(int i = 0; i < NumSamples; i++)
	{
		vec2 Xi = Hammersley(i, NumSamples);
		vec3 H = ImportanceSampleGGX(Xi, Roughness, N);
		vec3 L = 2 * dot(V, H) * H - V;
		float NoL = clamp(dot(N, L), 0.0, 1.0);
		if (NoL > 0)
		{
			PrefilteredColor += texture(u_EnvRadianceTex, L).rgb * NoL;
			TotalWeight += NoL;
		}
	}
	return PrefilteredColor / TotalWeight;
}

// -------------------- 工具函数 --------------------

// 绕Y轴旋转向量
vec3 RotateVectorAboutY(float angle, vec3 vec)
{
    angle = radians(angle);
    mat3x3 rotationMatrix ={vec3(cos(angle),0.0,sin(angle)),
                            vec3(0.0,1.0,0.0),
                            vec3(-sin(angle),0.0,cos(angle))};
    return rotationMatrix * vec;
}

// -------------------- 主PBR光照函数 --------------------

// 直接光照部分
vec3 Lighting(vec3 F0)
{
	vec3 result = vec3(0.0);
	for(int i = 0; i < LightCount; i++)
	{
		vec3 Li = -lights.Direction; // 光线方向
		vec3 Lradiance = lights.Radiance; // 光源辐射度
		vec3 Lh = normalize(Li + m_Params.View); // 半程向量

		// 计算各种夹角余弦
		float cosLi = max(0.0, dot(m_Params.Normal, Li));
		float cosLh = max(0.0, dot(m_Params.Normal, Lh));

		vec3 F = fresnelSchlick(F0, max(0.0, dot(Lh, m_Params.View)));
		float D = ndfGGX(cosLh, m_Params.Roughness);
		float G = gaSchlickGGX(cosLi, m_Params.NdotV, m_Params.Roughness);

		vec3 kd = (1.0 - F) * (1.0 - m_Params.Metalness); // 漫反射比例
		vec3 diffuseBRDF = kd * m_Params.Albedo;

		// Cook-Torrance高光项
		vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * m_Params.NdotV);

		result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
	}
	return result;
}

// 间接光照（IBL）部分
vec3 IBL(vec3 F0, vec3 Lr)
{
	vec3 irradiance = texture(u_EnvIrradianceTex, m_Params.Normal).rgb;
	vec3 F = fresnelSchlickRoughness(F0, m_Params.NdotV, m_Params.Roughness);
	vec3 kd = (1.0 - F) * (1.0 - m_Params.Metalness);
	vec3 diffuseIBL = m_Params.Albedo * irradiance;

	int u_EnvRadianceTexLevels = textureQueryLevels(u_EnvRadianceTex);
	float NoV = clamp(m_Params.NdotV, 0.0, 1.0);
	vec3 R = 2.0 * dot(m_Params.View, m_Params.Normal) * m_Params.Normal - m_Params.View;
	vec3 specularIrradiance = vec3(0.0);

	// 高光部分可选用预过滤或lod采样
	if (u_RadiancePrefilter > 0.5)
		specularIrradiance = PrefilterEnvMap(m_Params.Roughness * m_Params.Roughness, R) * u_RadiancePrefilter;
	else
		specularIrradiance = textureLod(u_EnvRadianceTex, RotateVectorAboutY(u_EnvMapRotation, Lr), sqrt(m_Params.Roughness) * u_EnvRadianceTexLevels).rgb * (1.0 - u_RadiancePrefilter);

	// BRDF查找表采样
	vec2 specularBRDF = texture(u_BRDFLUTTexture, vec2(m_Params.NdotV, 1.0 - m_Params.Roughness)).rg;
	vec3 specularIBL = specularIrradiance * (F * specularBRDF.x + specularBRDF.y);

	return kd * diffuseIBL + specularIBL;
}

// -------------------- 片元主函数 --------------------
void main()
{
	// 采样/设置PBR参数
	m_Params.Albedo = u_AlbedoTexToggle > 0.5 ? texture(u_AlbedoTexture, vs_Input.TexCoord).rgb : u_AlbedoColor; 
	m_Params.Metalness = u_MetalnessTexToggle > 0.5 ? texture(u_MetalnessTexture, vs_Input.TexCoord).r : u_Metalness;
	m_Params.Roughness = u_RoughnessTexToggle > 0.5 ?  texture(u_RoughnessTexture, vs_Input.TexCoord).r : u_Roughness;
    m_Params.Roughness = max(m_Params.Roughness, 0.05); // 保证最小粗糙度

	// 法线贴图或顶点法线
	m_Params.Normal = normalize(vs_Input.Normal);
	if (u_NormalTexToggle > 0.5)
	{
		m_Params.Normal = normalize(2.0 * texture(u_NormalTexture, vs_Input.TexCoord).rgb - 1.0);
		m_Params.Normal = normalize(vs_Input.WorldNormals * m_Params.Normal);
	}

	// 视线方向与NdotV
	m_Params.View = normalize(u_CameraPosition - vs_Input.WorldPosition);
	m_Params.NdotV = max(dot(m_Params.Normal, m_Params.View), 0.0);
		
	// 反射向量
	vec3 Lr = 2.0 * m_Params.NdotV * m_Params.Normal - m_Params.View;

	// 菲涅尔反射率，金属用Albedo
	vec3 F0 = mix(Fdielectric, m_Params.Albedo, m_Params.Metalness);

	// 直接光照和IBL
	vec3 lightContribution = Lighting(F0);
	vec3 iblContribution = IBL(F0, Lr);

	color = vec4(lightContribution + iblContribution, 1.0);
}