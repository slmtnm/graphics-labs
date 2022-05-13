SamplerState ObjSamplerState;
TextureCube SkyMap;

cbuffer SimpleConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
};

struct VS_INPUT
{
    float3 Pos : POSITION;
    float3 Norm: NORMAL;
    float4 Color : COLOR0;
};


struct VS_OUTPUT    //output structure for skymap vertex shader
{
    float4 Pos : SV_POSITION;
    float3 TexCoord : TEXCOORD;
};


VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    output.Pos = mul(float4(input.Pos, 1.0f), World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);

    output.TexCoord = input.Pos;

    return output;
}


float4 PS(VS_OUTPUT input) : SV_Target
{
    return SkyMap.Sample(ObjSamplerState, input.TexCoord);
}
