struct VSOut
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
};

float4 PS_Main(VSOut input)
	: SV_TARGET0
{
	return input.color;
}