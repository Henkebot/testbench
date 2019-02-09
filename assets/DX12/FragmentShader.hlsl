#define cbv(x) b##x
#ifdef DIFFUSE_SLOT
Texture2D g_texture : register(t3);
SamplerState g_sampler : register(s0);
#endif

struct VSOut
{
    float4 pos : SV_POSITION;
    float4 normal : NORMAL1;
    float2 uvs : TEXCOORD;
};


cbuffer CB1 : register(cbv(DIFFUSE_TINT))
{
    float4 diffuse;
}

float4 PS_Main(VSOut input)
	: SV_TARGET0
{
    float2 uv = input.uvs;
	//uv.y	  = 1.0f - uv.y;
#ifdef DIFFUSE_SLOT
	float4 col = g_texture.Sample(g_sampler, uv);
#else
    float4 col = float4(1, 1, 1, 1);
#endif
    return col * diffuse;
}