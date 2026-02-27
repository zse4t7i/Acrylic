struct InputPS
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

InputPS Main(float4 position: POSITION, float4 color: COLOR)
{
    InputPS result;

    result.position = position;
    result.color = color;

    return result;
}
