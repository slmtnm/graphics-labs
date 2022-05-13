SamplerState ObjSamplerState;
Texture2D Texture;

cbuffer SimpleConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
};

cbuffer ScreenSpaceConstantBuffer : register(b1)
{
    int isScreenSpace;
}

struct VS_INPUT
{
    float3 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};


struct VS_OUTPUT    //output structure for skymap vertex shader
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD;
};


VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    if (!isScreenSpace)
    {
        output.Pos = mul(float4(input.Pos, 1.0f), World);
        output.Pos = mul(output.Pos, View);
        output.Pos = mul(output.Pos, Projection);
    }
    else
        output.Pos = float4(input.Pos * 2, 1.0f);

    output.Tex = input.Tex;

    return output;
}


float4 PS(VS_OUTPUT input) : SV_Target
{
    return Texture.Sample(ObjSamplerState, input.Tex);
}
