
struct VS_INPUT
{
	float2 pos      : POSITION;
	float2 texCoord : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 pos      : SV_POSITION;
	float2 texCoord : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT vin)
{
	VS_OUTPUT vout;
	vout.pos = float4(vin.pos, 0.0f, 1.0f);
	vout.texCoord = vin.texCoord;
	return vout;
}