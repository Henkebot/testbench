#ifdef DIFFUSE_SLOT
Texture2D g_texture : register(t0, space1);
SamplerState g_sampler : register(s0);
#endif

struct VSOut
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
	float2 uvs : TEXCOORD;
};

float4 PS_Main(VSOut input)
	: SV_TARGET0
{
#ifdef DIFFUSE_SLOT
	return g_texture.Sample(g_sampler, input.uvs);
#endif
	return input.color;
}