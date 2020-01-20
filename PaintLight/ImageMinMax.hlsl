
#define REDUCTION_KERNEL_SIZE 512

Texture2D<float4>                    g_src       : register(t0);
shared RWStructuredBuffer<float4>    g_dstMaxRow : register(u0);
shared RWStructuredBuffer<float4>    g_dstMinRow : register(u1);
shared RWStructuredBuffer<float4>    g_dstMax    : register(u2);
shared RWStructuredBuffer<float4>    g_dstMin    : register(u3);

struct reductionInfo
{
	uint width, height;
	uint x_groups, y_groups;
};

cbuffer cb                                : register(b0)
{
	reductionInfo g_reductionInfo;
};

groupshared float4 g_groupDataMax[REDUCTION_KERNEL_SIZE];
groupshared float4 g_groupDataMin[REDUCTION_KERNEL_SIZE];

[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
}

[numthreads(REDUCTION_KERNEL_SIZE, 1, 1)]
void reduceRow(uint3 groupThreadID : SV_GroupThreadID, uint3 dispatchThreadId : SV_DispatchThreadID, uint3 groupIdx : SV_GroupID)
{
	uint global_x1 = min(dispatchThreadId.x, g_reductionInfo.width - 1);
	uint global_x2 = min(dispatchThreadId.x + REDUCTION_KERNEL_SIZE, g_reductionInfo.width - 1);
	g_groupDataMax[groupThreadID.x] = max(g_src.Load(uint3(global_x1, dispatchThreadId.yz)), g_src.Load(uint3(global_x2, dispatchThreadId.yz)));
	g_groupDataMin[groupThreadID.x] = min(g_src.Load(uint3(global_x1, dispatchThreadId.yz)), g_src.Load(uint3(global_x2, dispatchThreadId.yz)));
	GroupMemoryBarrierWithGroupSync();

	if (REDUCTION_KERNEL_SIZE >= 512)
	{
		if (groupThreadID.x < 256) {
			g_groupDataMax[groupThreadID.x] = max(g_groupDataMax[groupThreadID.x], g_groupDataMax[groupThreadID.x + 256]);
			g_groupDataMin[groupThreadID.x] = min(g_groupDataMin[groupThreadID.x], g_groupDataMin[groupThreadID.x + 256]);
		}
		GroupMemoryBarrierWithGroupSync();
	}

	if (REDUCTION_KERNEL_SIZE >= 256)
	{
		if (groupThreadID.x < 128) {
			g_groupDataMax[groupThreadID.x] = max(g_groupDataMax[groupThreadID.x], g_groupDataMax[groupThreadID.x + 128]);
			g_groupDataMin[groupThreadID.x] = min(g_groupDataMin[groupThreadID.x], g_groupDataMin[groupThreadID.x + 128]);
		}
		GroupMemoryBarrierWithGroupSync();
	}

	if (REDUCTION_KERNEL_SIZE >= 128)
	{
		if (groupThreadID.x < 64) {
			g_groupDataMax[groupThreadID.x] = max(g_groupDataMax[groupThreadID.x], g_groupDataMax[groupThreadID.x + 64]);
			g_groupDataMin[groupThreadID.x] = min(g_groupDataMin[groupThreadID.x], g_groupDataMin[groupThreadID.x + 64]);
		}
	}
	GroupMemoryBarrierWithGroupSync();

	if (groupThreadID.x < 32) {
		g_groupDataMax[groupThreadID.x] = max(g_groupDataMax[groupThreadID.x], g_groupDataMax[groupThreadID.x + 32]);
		g_groupDataMin[groupThreadID.x] = min(g_groupDataMin[groupThreadID.x], g_groupDataMin[groupThreadID.x + 32]);

		g_groupDataMax[groupThreadID.x] = max(g_groupDataMax[groupThreadID.x], g_groupDataMax[groupThreadID.x + 16]);
		g_groupDataMin[groupThreadID.x] = min(g_groupDataMin[groupThreadID.x], g_groupDataMin[groupThreadID.x + 16]);

		g_groupDataMax[groupThreadID.x] = max(g_groupDataMax[groupThreadID.x], g_groupDataMax[groupThreadID.x + 8]);
		g_groupDataMin[groupThreadID.x] = min(g_groupDataMin[groupThreadID.x], g_groupDataMin[groupThreadID.x + 8]);

		g_groupDataMax[groupThreadID.x] = max(g_groupDataMax[groupThreadID.x], g_groupDataMax[groupThreadID.x + 4]);
		g_groupDataMin[groupThreadID.x] = min(g_groupDataMin[groupThreadID.x], g_groupDataMin[groupThreadID.x + 4]);

		g_groupDataMax[groupThreadID.x] = max(g_groupDataMax[groupThreadID.x], g_groupDataMax[groupThreadID.x + 2]);
		g_groupDataMin[groupThreadID.x] = min(g_groupDataMin[groupThreadID.x], g_groupDataMin[groupThreadID.x + 2]);

		g_groupDataMax[groupThreadID.x] = max(g_groupDataMax[groupThreadID.x], g_groupDataMax[groupThreadID.x + 1]);
		g_groupDataMin[groupThreadID.x] = min(g_groupDataMin[groupThreadID.x], g_groupDataMin[groupThreadID.x + 1]);
	}

	if (groupThreadID.x == 0) {
		g_dstMaxRow[dispatchThreadId.y * g_reductionInfo.x_groups + groupIdx.x] = g_groupDataMax[0];
		g_dstMinRow[dispatchThreadId.y * g_reductionInfo.x_groups + groupIdx.x] = g_groupDataMin[0];
	}
}

[numthreads(1, REDUCTION_KERNEL_SIZE, 1)]
void reduceCol(uint3 groupThreadID : SV_GroupThreadID, uint3 dispatchThreadId : SV_DispatchThreadID, uint3 groupIdx : SV_GroupID)
{
	uint global_y1 = min(dispatchThreadId.y, g_reductionInfo.height - 1);
	uint global_y2 = min(dispatchThreadId.y + REDUCTION_KERNEL_SIZE, g_reductionInfo.height - 1);
	g_groupDataMax[groupThreadID.y] = max(g_dstMaxRow[g_reductionInfo.x_groups * global_y1 + dispatchThreadId.x], g_dstMaxRow[g_reductionInfo.x_groups * global_y2 + dispatchThreadId.x]);
	g_groupDataMin[groupThreadID.y] = min(g_dstMinRow[g_reductionInfo.x_groups * global_y1 + dispatchThreadId.x], g_dstMinRow[g_reductionInfo.x_groups * global_y2 + dispatchThreadId.x]);
	GroupMemoryBarrierWithGroupSync();

	if (REDUCTION_KERNEL_SIZE >= 512)
	{
		if (groupThreadID.y < 256) {
			g_groupDataMax[groupThreadID.y] = max(g_groupDataMax[groupThreadID.y], g_groupDataMax[groupThreadID.y + 256]);
			g_groupDataMin[groupThreadID.y] = min(g_groupDataMin[groupThreadID.y], g_groupDataMin[groupThreadID.y + 256]);
		}
		GroupMemoryBarrierWithGroupSync();
	}

	if (REDUCTION_KERNEL_SIZE >= 256)
	{
		if (groupThreadID.y < 128) {
			g_groupDataMax[groupThreadID.y] = max(g_groupDataMax[groupThreadID.y], g_groupDataMax[groupThreadID.y + 128]);
			g_groupDataMin[groupThreadID.y] = min(g_groupDataMin[groupThreadID.y], g_groupDataMin[groupThreadID.y + 128]);
		}
		GroupMemoryBarrierWithGroupSync();
	}

	if (REDUCTION_KERNEL_SIZE >= 128)
	{
		if (groupThreadID.y < 64) {
			g_groupDataMax[groupThreadID.y] = max(g_groupDataMax[groupThreadID.y], g_groupDataMax[groupThreadID.y + 64]);
			g_groupDataMin[groupThreadID.y] = min(g_groupDataMin[groupThreadID.y], g_groupDataMin[groupThreadID.y + 64]);
		}
		GroupMemoryBarrierWithGroupSync();
	}

	if (groupThreadID.y < 32) {
		g_groupDataMax[groupThreadID.y] = max(g_groupDataMax[groupThreadID.y], g_groupDataMax[groupThreadID.y + 32]);
		g_groupDataMin[groupThreadID.y] = min(g_groupDataMin[groupThreadID.y], g_groupDataMin[groupThreadID.y + 32]);

		g_groupDataMax[groupThreadID.y] = max(g_groupDataMax[groupThreadID.y], g_groupDataMax[groupThreadID.y + 16]);
		g_groupDataMin[groupThreadID.y] = min(g_groupDataMin[groupThreadID.y], g_groupDataMin[groupThreadID.y + 16]);

		g_groupDataMax[groupThreadID.y] = max(g_groupDataMax[groupThreadID.y], g_groupDataMax[groupThreadID.y + 8]);
		g_groupDataMin[groupThreadID.y] = min(g_groupDataMin[groupThreadID.y], g_groupDataMin[groupThreadID.y + 8]);

		g_groupDataMax[groupThreadID.y] = max(g_groupDataMax[groupThreadID.y], g_groupDataMax[groupThreadID.y + 4]);
		g_groupDataMin[groupThreadID.y] = min(g_groupDataMin[groupThreadID.y], g_groupDataMin[groupThreadID.y + 4]);

		g_groupDataMax[groupThreadID.y] = max(g_groupDataMax[groupThreadID.y], g_groupDataMax[groupThreadID.y + 2]);
		g_groupDataMin[groupThreadID.y] = min(g_groupDataMin[groupThreadID.y], g_groupDataMin[groupThreadID.y + 2]);

		g_groupDataMax[groupThreadID.y] = max(g_groupDataMax[groupThreadID.y], g_groupDataMax[groupThreadID.y + 1]);
		g_groupDataMin[groupThreadID.y] = min(g_groupDataMin[groupThreadID.y], g_groupDataMin[groupThreadID.y + 1]);
	}

	if (groupThreadID.y == 0) {
		g_dstMax[groupIdx.y * g_reductionInfo.x_groups + dispatchThreadId.x] = g_groupDataMax[0];
		g_dstMin[groupIdx.y * g_reductionInfo.x_groups + dispatchThreadId.x] = g_groupDataMin[0];
	}
}
