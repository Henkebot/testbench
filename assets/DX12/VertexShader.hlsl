
struct Data
{
	float4 position : SV_POSITION;
};

Data VS_main(uint vertexId : SV_VERTEXID)
{
	Data result;
    result.position = float4(0,0,0,0);
  

	return result;
}