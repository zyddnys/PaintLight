
#define GROUP_THREADS 32

Texture2D<float4>   g_src : register(t0);
RWTexture2D<float4> g_dst : register(u0);

struct textureInfo
{
	uint width, height;
	uint pad1, pad2;
};

cbuffer cb : register(b0)
{
	textureInfo     g_texInfo;
};

[numthreads(GROUP_THREADS, GROUP_THREADS, 1)]
void main(uint3 groupThreadID : SV_GroupThreadID, uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint3 cur_pos = dispatchThreadId.xyz;
	float3 color = g_src.Load(cur_pos).rgb;
	uint2 dst_pos = cur_pos.xy;
	dst_pos.x = g_texInfo.width - dst_pos.x - 1;
	g_dst[dst_pos] = float4(color, 255.0f);
	/*int2 shift = int2(500, 100);
	int3 src = int3(int2(dispatchThreadId.xy) + shift, dispatchThreadId.z);
	if (src.x >= int(g_texInfo.width))
		src.x = g_texInfo.width - (src.x % g_texInfo.width) - 1;
	if (src.x < 0)
		src.x = ((-src.x) % g_texInfo.width);
	if (src.y >= int(g_texInfo.height))
		src.y = g_texInfo.height - (src.y % g_texInfo.height) - 1;
	if (src.y < 0)
		src.y = ((-src.y) % g_texInfo.height);
	g_dst[dispatchThreadId.xy] = g_src[src.xy];*/
}
