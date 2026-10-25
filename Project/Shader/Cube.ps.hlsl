Texture2D t0 : register(t0);
Texture2D t1 : register(t1);
Texture2D t2 : register(t2);
SamplerState s0 : register(s0);

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

float4 Main(VS_OUTPUT input) : SV_TARGET
{
    return t0.Sample(s0, input.texCoord) * Color;
}
