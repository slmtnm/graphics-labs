//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    matrix Translation;
}


//--------------------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR0;
    float2 Tex : TEXCOORD0;
};


VS_OUTPUT VS(float4 Pos : POSITION, float4 Color : COLOR, float2 Tex : TEXCOORD0)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    output.Pos = mul(output.Pos, Translation);
    output.Pos = mul(Pos, World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Color = Color;
    output.Tex = Tex;
    return output;
}

Texture2D tex : register(t0);
SamplerState samLinear : register(s0);


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(VS_OUTPUT input) : SV_Target
{
    float4 color = tex.Sample(samLinear, input.Tex);
    float brightness = color[0] * 0.7 + color[1] * 0.2 + color[2] * 0.1;
    return float4(brightness, brightness, brightness, 1);
}
