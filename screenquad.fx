Texture2D tex : register(t0);
SamplerState samLinear : register(s0);

cbuffer BrightConstantBuffer : register(b0)
{
    int needExp;
    int _dummy[3];
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
    return needExp ? float4(exp(color[0]) - 1, exp(color[1]) - 1, exp(color[2]) - 1, 1) : color;
}
