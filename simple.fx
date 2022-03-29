//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer SimpleConstantBuffer : register( b0 )
{
    matrix World;
    matrix View;
    matrix Projection;
    float4 LightPos[4];
    float4 LightDir[4];
    float4 LightCutoff;
    float4 LightIntensity;
    int _dummy[24];
}

//--------------------------------------------------------------------------------------
// Vertex Shader's input and output vertex format
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float3 Pos : POSITION;
    float3 Norm : NORMAL;
    float4 Color : COLOR0;
};

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float3 Norm : TEXCOORD0;
    float3 WorldPos: TEXCOORD1;
    float4 Color : COLOR0;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VS( VS_INPUT input )
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    output.Pos = mul( float4(input.Pos, 1.0f), World );
    output.Pos = mul( output.Pos, View );
    output.Pos = mul( output.Pos, Projection );
    output.Color = input.Color;
    output.Norm = input.Norm;
    output.WorldPos = mul(input.Pos, World);

    return output;
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(VS_OUTPUT input) : SV_Target
{
    float4 resultColor = input.Color;

    for (uint i = 0; i < 3; i++) {
		// light color
		float4 lightColor = float4(1.0f, 1.0f, 1.0f, 1.0f) * LightIntensity[i];

		// ambient component
		float ambientStrength = 0.1;
		float4 ambient = lightColor * ambientStrength;

        // direction from point to light
        float3 lightDir = normalize(LightPos[i].xyz - input.WorldPos.xyz);

		float theta = dot(LightDir[i], - lightDir);
		if (theta > LightCutoff[i]) {
            // diffuse component
            float4 diffuse = max(dot(input.Norm, lightDir), 0.0) * float4(1.0f, 1.0f, 1.0f, 1.0f);
            resultColor *= ambient + diffuse;
        }
    }

    return resultColor;
}
