StructuredBuffer<float4> position : register(t0);
StructuredBuffer<float3> normal : register(t1);
StructuredBuffer<float2> uv : register(t2);

struct Data
{
	float4 position : SV_POSITION;
};

Data VS_Main(uint vertexId : SV_VertexId)
{
	Data result;
	result.position = position[vertexId];

	return result;
}