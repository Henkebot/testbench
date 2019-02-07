#ifdef DIFFUSE_SLOT

Texture2D g_texture : register(t3);
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
	float2 uv = input.uvs;
	//uv.y	  = 1.0f - uv.y;
#ifdef DIFFUSE_SLOT
	
	return g_texture.Sample(g_sampler, uv);

#endif
	//return float4(uv, 0, 1.0f);
	return input.color;
}