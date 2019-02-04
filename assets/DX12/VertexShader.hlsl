StructuredBuffer<float4> position : register(t0);
StructuredBuffer<float4> normal : register(t1);
StructuredBuffer<float2> uvs : register(t2);

struct VSOut
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
};

cbuffer CB : register(b0)
{
	float R, G, B, A;
}

VSOut VS_Main(in uint vertexID : SV_VertexID)
{
	VSOut output = (VSOut)0;
	
	output.pos = normal[vertexID];
	//
	output.color = float4(1, 1, 1, 1);

	return output;
}