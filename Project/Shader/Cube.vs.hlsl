struct VS_INPUT
{
    float3 pos : POSITION;
    float2 texCoord: TEXCOORD;
};

struct VS_OUTPUT
{
    float4 pos: SV_POSITION;
    float2 texCoord: TEXCOORD;
};

cbuffer ConstantBuffer : register(b0)
{
    float4x4 MVP;
    float4 Color;
};

VS_OUTPUT Main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.pos = mul(float4(input.pos, 1.0f), MVP);
    output.texCoord = input.texCoord;
    return output;
}