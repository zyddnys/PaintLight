
#define THREAD_COUNT 32

Texture2D<float4>   g_srcImage            : register(t0);
Texture2D<float4>   g_srcStrokeDensity    : register(t1);

RWTexture2D<float4> g_dstSobelX           : register(u0);
RWTexture2D<float4> g_dstSobelY           : register(u1);
RWTexture2D<float4> g_dstSobel            : register(u2);
RWTexture2D<float4> g_srcSobelXnormalized : register(u3);
RWTexture2D<float4> g_srcSobelYnormalized : register(u4);
RWTexture2D<float4> g_dst                 : register(u5);

struct coarseLightingInfo
{
	uint width, height;
	float light_source_x, light_source_y, light_source_z;
	float pixel_scale;
	float delta_pd;
	uint pad1;
};

cbuffer cb                : register(b0)
{
	coarseLightingInfo g_info;
}

groupshared float4 gCache[THREAD_COUNT + 2][THREAD_COUNT + 2];

/*[numthreads(THREAD_COUNT, THREAD_COUNT, 1)]
void sobel(uint3 groupThreadID : SV_GroupThreadID, uint3 dispatchThreadId : SV_DispatchThreadID)
{
	if (groupThreadID.y == 0) {
		gCache[0][groupThreadID.x + 1] = g_srcImage.Load(uint3(dispatchThreadId.x, dispatchThreadId.y - 1, dispatchThreadId.z));
	}
	if (groupThreadID.y == THREAD_COUNT - 1) {
		gCache[THREAD_COUNT + 1][groupThreadID.x + 1] = g_srcImage.Load(uint3(dispatchThreadId.x, dispatchThreadId.y + 1, dispatchThreadId.z));
	}
	if (groupThreadID.x == 0 && groupThreadID.y == 0) {
		gCache[0][0] = g_srcImage.Load(uint3(dispatchThreadId.x - 1, dispatchThreadId.y - 1, dispatchThreadId.z));
	}
	if ((groupThreadID.x == THREAD_COUNT - 1) && groupThreadID.y == 0) {
		gCache[0][THREAD_COUNT + 1] = g_srcImage.Load(uint3(dispatchThreadId.x + 1, dispatchThreadId.y - 1, dispatchThreadId.z));
	}

	if (groupThreadID.x == 0) {
		gCache[groupThreadID.y + 1][0] = g_srcImage.Load(uint3(dispatchThreadId.x - 1, dispatchThreadId.yz));
	}
	if (groupThreadID.x == THREAD_COUNT - 1) {
		gCache[groupThreadID.y + 1][THREAD_COUNT + 1] = g_srcImage.Load(uint3(dispatchThreadId.x + 1, dispatchThreadId.yz));
	}
	if (groupThreadID.x == 0 && (groupThreadID.y == THREAD_COUNT - 1)) {
		gCache[0][THREAD_COUNT + 1] = g_srcImage.Load(uint3(dispatchThreadId.x - 1, dispatchThreadId.y + 1, dispatchThreadId.z));
	}
	if ((groupThreadID.x == THREAD_COUNT - 1) && (groupThreadID.y == THREAD_COUNT - 1)) {
		gCache[THREAD_COUNT + 1][THREAD_COUNT + 1] = g_srcImage.Load(uint3(dispatchThreadId.x + 1, dispatchThreadId.y + 1, dispatchThreadId.z));
	}

	gCache[groupThreadID.y + 1][groupThreadID.x + 1] = g_srcImage.Load(dispatchThreadId);

	GroupMemoryBarrierWithGroupSync();

	float4 sumX = float4(0.0f, 0.0f, 0.0f, 0.0f);
	sumX += -1.0f * gCache[groupThreadID.y - 1][groupThreadID.x - 1];
	sumX += +1.0f * gCache[groupThreadID.y - 1][groupThreadID.x + 1];
	sumX += -2.0f * gCache[groupThreadID.y][groupThreadID.x - 1];
	sumX += +2.0f * gCache[groupThreadID.y][groupThreadID.x + 1];
	sumX += -1.0f * gCache[groupThreadID.y + 1][groupThreadID.x - 1];
	sumX += +1.0f * gCache[groupThreadID.y + 1][groupThreadID.x + 1];

	float4 sumY = float4(0.0f, 0.0f, 0.0f, 0.0f);
	sumY += -1.0f * gCache[groupThreadID.y - 1][groupThreadID.x - 1];
	sumY += -2.0f * gCache[groupThreadID.y - 1][groupThreadID.x];
	sumY += -1.0f * gCache[groupThreadID.y - 1][groupThreadID.x + 1];
	sumY += +1.0f * gCache[groupThreadID.y + 1][groupThreadID.x - 1];
	sumY += +2.0f * gCache[groupThreadID.y + 1][groupThreadID.x];
	sumY += +1.0f * gCache[groupThreadID.y + 1][groupThreadID.x + 1];

	g_dstSobelX[uint2(dispatchThreadId.x, dispatchThreadId.y)] = sumX + 1e-10;
	g_dstSobelY[uint2(dispatchThreadId.x, dispatchThreadId.y)] = sumY + 1e-10;
	g_dstSobel[uint2(dispatchThreadId.x, dispatchThreadId.y)] = sqrt(sumX * sumX + sumY * sumY);
}*/

[numthreads(THREAD_COUNT, THREAD_COUNT, 1)]
void sobel(uint3 groupThreadID : SV_GroupThreadID, uint3 dispatchThreadId : SV_DispatchThreadID)
{
	float4 a11 = g_srcImage.Load(uint3(max(dispatchThreadId.x - 1, 0), max(dispatchThreadId.y - 1, 0), dispatchThreadId.z));
	float4 a12 = g_srcImage.Load(uint3(dispatchThreadId.x, max(dispatchThreadId.y - 1, 0), dispatchThreadId.z));
	float4 a13 = g_srcImage.Load(uint3(min(dispatchThreadId.x + 1, g_info.width - 1), max(dispatchThreadId.y - 1, 0), dispatchThreadId.z));

	float4 a21 = g_srcImage.Load(uint3(max(dispatchThreadId.x - 1, 0), dispatchThreadId.y, dispatchThreadId.z));
	float4 a22 = g_srcImage.Load(uint3(dispatchThreadId.x, dispatchThreadId.y, dispatchThreadId.z));
	float4 a23 = g_srcImage.Load(uint3(min(dispatchThreadId.x + 1, g_info.width - 1), dispatchThreadId.y, dispatchThreadId.z));

	float4 a31 = g_srcImage.Load(uint3(max(dispatchThreadId.x - 1, 0), min(dispatchThreadId.y + 1, g_info.height - 1), dispatchThreadId.z));
	float4 a32 = g_srcImage.Load(uint3(dispatchThreadId.x, min(dispatchThreadId.y + 1, g_info.height - 1), dispatchThreadId.z));
	float4 a33 = g_srcImage.Load(uint3(min(dispatchThreadId.x + 1, g_info.width - 1), min(dispatchThreadId.y + 1, g_info.height - 1), dispatchThreadId.z));

	float4 sumX = float4(0.0f, 0.0f, 0.0f, 0.0f);
	sumX += -1.0f * a11;
	sumX +=  1.0f * a13;
	sumX += -2.0f * a21;
	sumX +=  2.0f * a23;
	sumX += -1.0f * a31;
	sumX +=  1.0f * a33;

	float4 sumY = float4(0.0f, 0.0f, 0.0f, 0.0f);
	sumY += -1.0f * a11;
	sumY += -2.0f * a12;
	sumY += -1.0f * a13;
	sumY +=  1.0f * a31;
	sumY +=  2.0f * a32;
	sumY +=  1.0f * a33;

	g_dstSobelX[uint2(dispatchThreadId.x, dispatchThreadId.y)] = sumX + 1e-10;
	g_dstSobelY[uint2(dispatchThreadId.x, dispatchThreadId.y)] = sumY + 1e-10;
	g_dstSobel[uint2(dispatchThreadId.x, dispatchThreadId.y)] = sqrt(sumX * sumX + sumY * sumY);
}

[numthreads(THREAD_COUNT, THREAD_COUNT, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	float density_scaled = clamp(g_srcStrokeDensity.Load(dispatchThreadId).r, 0.0f, 1.0f);
	float density = sqrt(1.0f - density_scaled * density_scaled + 1e-10);
	float3 sobelX = g_srcSobelXnormalized.Load(dispatchThreadId).rgb;
	float3 sobelY = g_srcSobelYnormalized.Load(dispatchThreadId).rgb;

	float lz = g_info.light_source_z;
	float lx = g_info.light_source_x;
	float ly = g_info.light_source_y;
	float ln = length(float3(lx, ly, lz));
	lz /= ln;
	lx /= ln;
	ly /= ln;

	float3 iz = float3(density_scaled, density_scaled, density_scaled);
	float3 ix = sobelX * density;
	float3 iy = sobelY * density;

	float3 final_effect = iz * lz + ix * lx + iy * ly;

	g_dst[dispatchThreadId.xy] = float4(final_effect, 1.0f);
}
