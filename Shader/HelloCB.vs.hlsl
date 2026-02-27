struct InputPS
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
};

cbuffer CB : register(b0)
{
    float4 Offset;
    float4 padding[15];
};

InputPS Main(float4 position: POSITION, float2 uv: TEXCOORD)
{
    InputPS result;

    result.Position = position + Offset;
    result.UV = uv;

    return result;
}
