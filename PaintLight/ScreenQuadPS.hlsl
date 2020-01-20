
Texture2D<float4> g_tex    : register(t0);
SamplerState g_tex_sampler : register(s0);

struct VS_OUTPUT
{
	float4 pos      : SV_POSITION;
	float2 texCoord : TEXCOORD0;
};

struct screenQuadInfo
{
	float rangeMin, rangeMax;
	float gamma;
	uint pad;
};

cbuffer cb                : register(b0)
{
	screenQuadInfo g_info;
};

float4 main(VS_OUTPUT pin) : SV_TARGET
{
	float4 color = g_tex.Sample(g_tex_sampler, pin.texCoord);
	float3 val = (color.xyz - g_info.rangeMin) / g_info.rangeMax;
	return float4(pow(val, g_info.gamma), 1.0f);
}
