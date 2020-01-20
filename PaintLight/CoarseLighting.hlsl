
#define GROUP_THREADS 32

Texture2D<float4>   g_src : register(t0);
RWTexture2D<float4> g_dst : register(u0);

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

float3 bilinearSample(float2 pos)
{
	float x = pos.x;
	float y = float(g_info.height) - pos.y - 1.0f;

	float x0 = floor(x);
	float x1 = x0 + 1.0f;
	float y0 = floor(y);
	float y1 = y0 + 1.0f;

	float x0a = clamp(x0, 0.0f, float(g_info.width - 1));
	float x1a = clamp(x1, 0.0f, float(g_info.width - 1));
	float y0a = clamp(y0, 0.0f, float(g_info.height - 1));
	float y1a = clamp(y1, 0.0f, float(g_info.height - 1));

	float3 Ia = g_src.Load(uint3(x0a, y0a, 0)).rgb;
	float3 Ib = g_src.Load(uint3(x0a, y1a, 0)).rgb;
	float3 Ic = g_src.Load(uint3(x1a, y0a, 0)).rgb;
	float3 Id = g_src.Load(uint3(x1a, y1a, 0)).rgb;

	float wa = (x1 - x) * (y1 - y);
	float wb = (x1 - x) * (y - y0);
	float wc = (x - x0) * (y1 - y);
	float wd = (x - x0) * (y - y0);

	return Ia * wa + Ib * wb + Ic * wc + Id * wd;
}

[numthreads(GROUP_THREADS, GROUP_THREADS, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	float delta_pd = g_info.delta_pd;
	int x = dispatchThreadId.x;
	int y = g_info.height - dispatchThreadId.y - 1;

	float pd = sqrt((y - g_info.light_source_y) * (y - g_info.light_source_y) + (x - g_info.light_source_x) * (x - g_info.light_source_x));
	float sin_theta = (y - g_info.light_source_y) / pd;
	float cos_theta = (x - g_info.light_source_x) / pd;

	float light_dir_y = g_info.light_source_z;
	float light_dir_x = pd;
	float light_dir_len = sqrt(light_dir_y * light_dir_y + light_dir_x * light_dir_x);
	light_dir_y /= light_dir_len;
	light_dir_x /= light_dir_len;

	float3 n_pd = bilinearSample(float2(g_info.light_source_x + pd * cos_theta, g_info.light_source_y + pd * sin_theta));
	float3 n_delta_pd = bilinearSample(float2(g_info.light_source_x + (pd + delta_pd) * cos_theta, g_info.light_source_y + (pd + delta_pd) * sin_theta));
	float3 surface_dir_y = (n_delta_pd - n_pd) * g_info.pixel_scale;

	float3 surface_dir_x = float3(delta_pd, delta_pd, delta_pd);
	float3 surface_dir_len = sqrt(surface_dir_y * surface_dir_y + surface_dir_x * surface_dir_x);
	surface_dir_y /= surface_dir_len;
	surface_dir_x /= surface_dir_len;

	float3 e = light_dir_x * surface_dir_x + light_dir_y * surface_dir_y;

	g_dst[dispatchThreadId.xy] = float4(e, 255.0f);
}
