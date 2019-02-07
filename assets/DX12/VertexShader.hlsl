StructuredBuffer<float4> position : register(t0);
StructuredBuffer<float4> normal : register(t1);
StructuredBuffer<float2> uvs : register(t2);

struct VSOut
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
	float2 uvs : TEXCOORD;
};


cbuffer CB1 : register(b6)
{
	float4 diffuse;
}
cbuffer CB : register(b5)
{
	float4 translate;
}


VSOut VS_Main(in uint vertexID : SV_VertexID, in uint instanceID : SV_InstanceID)
{
	VSOut output = (VSOut)0;
	
	output.pos = position[vertexID] + translate;
	
	output.color = diffuse;
	output.uvs   = uvs[vertexID];

	return output;
}