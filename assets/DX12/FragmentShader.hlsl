struct VSOut
{
	float4 pos : SV_POSITION;
};

float4 PS_Main(VSOut input)
	: SV_TARGET
{
    return float4(1, 1, 1, 1);
}