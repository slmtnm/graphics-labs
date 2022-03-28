Texture2D tex : register(t0);
SamplerState samLinear : register(s0);

cbuffer BrightnessConstantBuffer : register(b0)
{
    int isBrightnessCalc;
    int _dummy[15];
}

//--------------------------------------------------------------------------------------

struct VS_INPUT
{
    float4 Pos : POSITION;
    float4 Color : COLOR0;
    float2 Tex : TEXCOORD0;
};


struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR0;
    float2 Tex : TEXCOORD0;
};


VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    output.Pos = input.Pos;
    output.Color = input.Color;
    output.Tex = input.Tex;
    return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(VS_OUTPUT input) : SV_TARGET
{
    float4 color = tex.Sample(samLinear, input.Tex);

    if (isBrightnessCalc)
    {
        float brightness = log(color[0] * 0.2126 + color[1] * 0.7151 + color[2] * 0.0722 + 1.0f);
        return float4(brightness, brightness, brightness, 1);
    }
    return color;
}
