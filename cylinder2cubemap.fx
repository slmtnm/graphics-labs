SamplerState ObjSamplerState;
Texture2D Texture;

cbuffer SimpleConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
};

struct VS_INPUT
{
    float3 Pos : POSITION;
    float3 Norm : NORMAL;
    float4 Color : COLOR0;
};


struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float3 WorldPos : TEXCOORD;
};


VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    output.Pos = mul(float4(input.Pos, 1.0f), World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    //output.Pos = float4(input.Pos, 1.0f);
    
    output.WorldPos = output.Pos.xyz;

    return output;
}


float4 PS(VS_OUTPUT input) : SV_Target
{
    const float PI = 3.14159265359f;

    float3 pos = normalize(input.WorldPos);
    float u = atan2(pos.x, pos.z + 1e-4f) / (2 * PI);
    float v = 1.0f - (asin(pos.y) / PI - 0.5f);
    return Texture.Sample(ObjSamplerState, float2(u, v));
}
