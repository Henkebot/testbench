struct VSOut
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
};

cbuffer CB : register(b0)
{
	float R, G, B, A;
}

VSOut VS_Main()
{
	VSOut output = (VSOut)0;
	
	output.pos   = float4(1, 0, 0, 1.0f);
	output.color = float4(1, 1, 1, 1);

	return output;
}