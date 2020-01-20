
#define GROUP_THREADS 32

Texture2D<float4>   g_src : register(t0);
RWTexture2D<float4> g_dst : register(u0);

struct scalar
{
	float3 value;
	uint pad;
};

cbuffer cb                : register(b0)
{
	scalar g_scalar;
}

[numthreads(GROUP_THREADS, GROUP_THREADS, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	float3 color = g_src.Load(dispatchThreadId).rgb;
	color *= g_scalar.value;
	g_dst[dispatchThreadId.xy] = float4(color, 255.0f);
}
