
#define GROUP_THREADS 32

Texture2D<float4>   g_src1 : register(t0);
Texture2D<float4>   g_src2 : register(t1);
RWTexture2D<float4> g_dst  : register(u0);

[numthreads(GROUP_THREADS, GROUP_THREADS, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	float3 color1 = g_src1.Load(dispatchThreadId).rgb;
	float3 color2 = g_src2.Load(dispatchThreadId).rgb;
	g_dst[dispatchThreadId.xy] = float4(color1 * color2, 255.0f);
}
