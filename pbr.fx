//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer PBRConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    // lights
    float4 LightColor[4];
    float4 LightPos[4];
    float4 LightDir[4];
    float4 LightCutoff;
    float4 LightIntensity;
    // camera
    float3 CameraPos;
}

cbuffer MaterialConstantBuffer : register(b1)
{
    float3 F0;
    float roughness;
    float metalness;
}


const float PI = 3.14159f;

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
    float3 WorldCamPos : TEXCOORD2;
    float4 Color : COLOR0;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    output.Pos = mul(float4(input.Pos, 1.0f), World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Color = input.Color;
    output.Norm = input.Norm;
    output.WorldPos = mul(float4(input.Pos, 1.0f), World);
    output.WorldCamPos = mul(float4(CameraPos, 1.0f), World);

    return output;
}


float pow2(float x)
{
    return x * x;
}

float pow5(float x)
{
    return x * pow2(pow2(x));
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float D(float3 n, float3 h)
{
    float Dval = pow2(roughness) / (PI * pow2(pow2(n * h) * (pow2(roughness) - 1) + 1))
    return Dval;
}

float Gv(float3 n, float3 vec)
{
    float k = (roughness + 1) * (roughness + 1) / 8;
    float nv = n * vec;
    float Gval = nv / (nv * (1 - k) + k);
    return Gval;
}

float G(float3 n, float3 v, float3 l)
{
    return Gv(n, v) * Gv(n, l);
}

float3 F(float3 h, float3 v)
{
    float3 F0met = float3(0.04f, 0.04f, 0.04f) * (1 - metalness) + F0 * metalness;
    float3 Fval = F0met + (float3(1.0f, 1.0f, 1.0f) - F0met) * pow5(1 - h * v);
    return Fval;
}

float fr(float3 albedo, float3 lightDir, float3 camDir, float3 n)
{
    float3 wi = lightDir; // alias
    float3 wo = 2 * n * (wi * n) - wi;
    float3 l = lightDir; // alias
    float3 v = camDir; // alias
    float3 h = normalize((v + l) * 0.5f);

    float3 Fval = F(h, v);
    float3 Dval = D(n, h);
    float3 Gval = G(n, v, l);

    float3 frval =
        Fval * albedo / PI * (1 - metalness) +
        Dval * Fval * Gval / (4 * (wi * n) * (wo * n));
    return frval;
}

float4 PS(VS_OUTPUT input) : SV_Target
{
    float3 resultColor = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < 3; i++) {
        // direction from point to light
        float3 lightDir = normalize(LightPos[i].xyz - input.WorldPos.xyz);
        // direction from point to camera
        float3 camDir = normalize(input.WorldCamPos.xyz - input.WorldPos.xyz);
        // light color
        float3 lightColor = LightColor[i] * LightIntensity[i];
        // normal
        float3 n = normalize(input.Norm);
        // result color
        float3 color = fr(input.Color, lightDir, camDir, n) * lightColor * (lightDir * n);

        resultColor += color;
    }

    return resultColor;
}
