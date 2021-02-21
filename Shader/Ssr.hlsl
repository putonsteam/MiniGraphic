struct VSInput
{
	float3 PosL    : POSITION;
	float3 NormalL : NORMAL;
	float2 TexC    : TEXCOORD;
};

struct PSInput
{
	float4 position 	: SV_POSITION;
	float3 positionW	: positionW;		// World space position
};

cbuffer FrameBuffer : register(b0)
{
	float4x4 mView;
	float4x4 mViewProj;
	float3   vEyePos;
};

// Nonnumeric values cannot be added to a cbuffer.
Texture2D WorldPosTex    : register(t0);
Texture2D gNormalMap     : register(t1);
Texture2D DeferredTex    : register(t2);
SamplerState gsamPointClamp : register(s0);
SamplerState gsamLinearClamp : register(s1);
SamplerState gsamDepthMap : register(s2);
SamplerState gsamLinearWrap : register(s3);

//Texture2D WorldPosTex : register(t0);
//Texture2D g_texture2 : register(t2);
//TextureCube g_EnvironmentLight : register(t3);
//Texture2D g_texture5 : register(t5);
//SamplerState g_sampler : register(s0);

struct PsOutput
{
	float4 color 	: SV_TARGET;
	//float depth 	: SV_DEPTH;
};

PSInput VS(VSInput input)
{
	PSInput result;

	float3 position = input.PosL;
	position.y += 1.0f;
	result.position = mul(float4(position, 1.0f), mViewProj);

	result.positionW = position;

	return result;
}



PsOutput PS(PSInput input)
{
	PsOutput result;

	float3 p = input.positionW;//float3(0.0,-8.0f,60.0f);//

	float3 vToPoint = normalize(p - vEyePos.xyz);
	float3 vReflection = reflect(vToPoint, float3(0, 1, 0));

	uint nNum = 40;
	float fStep = 40.0 / nNum;

	//
	float4 color = float4(0, 0, 0, 0);
	float value = 1.0f;
	for (uint i = 1; i < nNum; ++i)
	{
		float3 vPointW = p + vReflection * i*fStep;

		float4 vPointP = mul(float4(vPointW, 1.0f), mViewProj);
		vPointP /= vPointP.w;

		float2 uv = vPointP.xy;
		uv += 1.0f;
		uv /= 2.0f;
		uv.y = 1.0f - uv.y;

		//float4 tex = g_texture2.Sample(g_sampler, uv);
		//if (tex.a != 0.0f)
		//{
			float depthR = mul(float4(vPointW, 1.0f), mView).z;
			if (depthR <= -60.0f)
				continue;

			float3 positionT = WorldPosTex.Sample(gsamLinearClamp, uv).xyz;
			float depthT = mul(float4(positionT, 1.0f), mView).z;

			if (depthT > depthR)
			{
				color = DeferredTex.Sample(gsamLinearClamp, uv);
				value = (float)i / (float)nNum;
				break;
			}
		//}

	}

	//float4 environment = g_EnvironmentLight.SampleLevel(g_sampler, vReflection, 0);
	//color = lerp(color, environment, value);

	result.color = float4(color.xyz, 1.0f);

	//
	//float4 position = mul(float4(input.positionW.xyz, 1.0f), mViewProj);
	//result.depth = position.z/position.w;

	return result;
}
