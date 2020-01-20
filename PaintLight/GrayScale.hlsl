
#define GROUP_THREADS 32

Texture2D<float4>   g_src : register(t0);
RWTexture2D<float4> g_dst : register(u0);

[numthreads(GROUP_THREADS, GROUP_THREADS, 1)]
void main(uint3 groupThreadID : SV_GroupThreadID, uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint3 cur_pos = dispatchThreadId;
	float3 colorful = g_src.Load(cur_pos).rgb;
	float avg = (colorful.x + colorful.y + colorful.z) / 3.0f;
	float3 gray = float3(avg, avg, avg);
	g_dst[cur_pos.xy] = float4(gray, 255.0f);
}
