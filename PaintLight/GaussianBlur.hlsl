#define GROUP_THREADS 256
#define MAX_RADIUS 92

Texture2D<float4>             g_src    : register(t0);
StructuredBuffer<float>       g_kernel : register(t1);
shared RWTexture2D<float4>    g_dst    : register(u0);
shared RWTexture2D<float4>    g_horDst : register(u1);

struct gaussianBlurInfo
{
	int radius;
	int width, height;
	uint pad1;
};

cbuffer cb                         : register(b0)
{
	gaussianBlurInfo     g_info;
};

[numthreads(1, 1, 1)]
void main(uint3 groupThreadID : SV_GroupThreadID, uint3 dispatchThreadId : SV_DispatchThreadID) // 
{

}

#define CACHE_SIZE (GROUP_THREADS + 2 * MAX_RADIUS)
groupshared float4 gCache[CACHE_SIZE];

/*[numthreads(GROUP_THREADS, 1, 1)]
void mainH(uint3 groupThreadID : SV_GroupThreadID, uint3 dispatchThreadId : SV_DispatchThreadID) // 
{
	int radius = g_info.radius;
	// horizontal blur
	int3 pos = dispatchThreadId;
	float3 sum = float3(0.0f, 0.0f, 0.0f);
	for (int i = -radius; i <= radius; ++i)
	{
		//int x = min(max(pos.x + i, 0), g_info.width - 1);
        int x = pos.x + i;
        if (x >= g_info.width)
            x = g_info.width - (x % g_info.width) - 1;
        if (x < 0)
            x = ((-x) % g_info.width);
		float3 pixel_value = g_src.Load(uint3(x, pos.y, pos.z)).rgb;
		sum += pixel_value * g_kernel.Load(i + radius);
	}
	g_dst[dispatchThreadId.xy] = float4(sum, 255.0f);
}

[numthreads(1, GROUP_THREADS, 1)]
void mainV(uint3 groupThreadID : SV_GroupThreadID, uint3 dispatchThreadId : SV_DispatchThreadID) // 
{
	int radius = g_info.radius;
	// vertical blur
	int3 pos = dispatchThreadId;
	float3 sum = float3(0.0f, 0.0f, 0.0f);
	for (int i = -radius; i <= radius; ++i)
	{
		//int y = min(max(pos.y + i, 0), g_info.height - 1);
        int y = pos.y + i;
        if (y >= g_info.height)
            y = g_info.height - (y % g_info.height) - 1;
        if (y < 0)
            y = ((-y) % g_info.height);
		float3 pixel_value = g_dst.Load(uint3(pos.x, y, pos.z)).rgb;
		sum += pixel_value * g_kernel.Load(i + radius);
	}
	g_dst[dispatchThreadId.xy] = float4(sum, 255.0f);
}*/

[numthreads(GROUP_THREADS, 1, 1)]
void mainH(uint3 groupThreadID : SV_GroupThreadID, uint3 dispatchThreadId : SV_DispatchThreadID)
{
    int clamedDisX = min(dispatchThreadId.x, g_info.width - 1);
    int clamedDisY = min(dispatchThreadId.y, g_info.height - 1);
    int bRadius = g_info.radius;
    if (int(groupThreadID.x) < bRadius) //load left
    {
        int x = max(int(dispatchThreadId.x) - bRadius, 0);
        uint2 clampedPos = uint2(x, clamedDisY);
        gCache[groupThreadID.x] = g_src[clampedPos];
    }
    else if (int(groupThreadID.x) >= GROUP_THREADS - bRadius) // load right
    {
        int x = min(dispatchThreadId.x + bRadius, g_info.width - 1);
        uint2 clampedPos = uint2(x, clamedDisY);
        gCache[int(groupThreadID.x) + 2 * bRadius] = g_src[clampedPos];
    }

    uint2 clampedPos = uint2(clamedDisX, clamedDisY);
    gCache[groupThreadID.x + uint(bRadius)] = g_src[clampedPos];

    GroupMemoryBarrierWithGroupSync();

    float4 blurColor = gCache[groupThreadID.x + bRadius + bRadius] * g_kernel[bRadius + bRadius];
    for (int i = -bRadius; i < bRadius; i += 2)
    {
        int k = groupThreadID.x + bRadius + i;
        blurColor += gCache[k + 0] * g_kernel[i + bRadius + 0];
        blurColor += gCache[k + 1] * g_kernel[i + bRadius + 1];
    }
    g_horDst[dispatchThreadId.xy] = float4(blurColor.xyz, 255.0f);
}

[numthreads(1, GROUP_THREADS, 1)]
void mainV(uint3 groupThreadID : SV_GroupThreadID, uint3 dispatchThreadId : SV_DispatchThreadID)
{
    int clamedDisX = min(max(dispatchThreadId.x, 0), g_info.width - 1);
    int clamedDisY = min(max(dispatchThreadId.y, 0), g_info.height - 1);
    int bRadius = g_info.radius;
    if (int(groupThreadID.y) < bRadius)
    {
        int y = max(int(dispatchThreadId.y) - bRadius, 0);
        uint2 clampedPos = uint2(clamedDisX, y);
        gCache[groupThreadID.y] = g_horDst.Load(clampedPos);
    }
    else if (int(groupThreadID.y) >= GROUP_THREADS - bRadius)
    {
        int y = min(dispatchThreadId.y + bRadius, g_info.height - 1);
        uint2 clampedPos = uint2(clamedDisX, y);
        gCache[int(groupThreadID.y) + 2 * bRadius] = g_horDst.Load(clampedPos);
    }
    gCache[groupThreadID.y + uint(bRadius)] = g_horDst.Load(uint2(clamedDisX, clamedDisY));

    GroupMemoryBarrierWithGroupSync();

    float4 blurColor = gCache[groupThreadID.y + bRadius + bRadius] * g_kernel[bRadius + bRadius];
    for (int i = -bRadius; i < bRadius; i += 2)
    {
        int k = groupThreadID.y + bRadius + i;
        blurColor += gCache[k + 0] * g_kernel[i + bRadius + 0];
        blurColor += gCache[k + 1] * g_kernel[i + bRadius + 1];
    }
    g_dst[dispatchThreadId.xy] = float4(blurColor.xyz, 255.0f);
}
