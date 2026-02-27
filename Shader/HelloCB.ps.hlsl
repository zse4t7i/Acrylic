struct InputPS
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

float4 Main(InputPS input) : SV_TARGET
{
    return g_texture.Sample(g_sampler, input.UV);
}
