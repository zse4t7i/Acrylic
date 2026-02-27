struct InputPS
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

InputPS Main(float4 position: POSITION, float2 uv: TEXCOORD)
{
    InputPS result;

    result.position = position;
    result.uv = uv;

    return result;
}
