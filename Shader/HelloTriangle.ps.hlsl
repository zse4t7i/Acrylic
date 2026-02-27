struct InputPS
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

float4 Main(InputPS input) : SV_TARGET
{
    return input.color;
}
