Texture2D tex : register(t0);
SamplerState samLinear : register(s0);

cbuffer BrightnessConstantBuffer : register(b0)
{
    int isBrightnessCalc;
}

//--------------------------------------------------------------------------------------

struct VS_INPUT
{
    float3 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};


struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};


VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    output.Pos = float4(input.Pos, 1.0f);
    output.Tex = input.Tex;
    return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(VS_OUTPUT input) : SV_TARGET
{
    if (isBrightnessCalc)
    {
        float4 color = tex.Sample(samLinear, input.Tex);
        float brightness = log(color[0] * 0.2126 + color[1] * 0.7151 + color[2] * 0.0722 + 1.0f);
        return brightness;
    }

    float color = tex.Sample(samLinear, input.Tex);
    return color;
}
