#define srv(x) t##x
#define cbv(x) b##x
StructuredBuffer<float4> position : register(srv(POSITION));
StructuredBuffer<float4> normal : register(srv(NORMAL));
StructuredBuffer<float2> uvs : register(srv(TEXTCOORD));

struct VSOut
{
    float4 pos : SV_POSITION;
    float4 normal : NORMAL1;
    float2 uvs : TEXCOORD;
};


cbuffer CB : register(cbv(TRANSLATION))
{
    float4 translate;
}


VSOut VS_Main(in uint vertexID : SV_VertexID)
{
    VSOut output = (VSOut) 0;

    output.pos = position[vertexID] + translate;
    output.pos.z = -output.pos.z;
    
#ifdef NORMAL
    output.normal =normal[vertexID ];
#endif

#ifdef TEXTCOORD
    output.uvs = uvs[vertexID ];
#endif

    return output;
}